#!/usr/bin/env python3
# Copyright 2021 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""This script is used to fetch reclient cfgs."""

import argparse
import glob
import logging
import os
import posixpath
import re
import shutil
import string
import subprocess
import sys

THIS_DIR = os.path.abspath(os.path.dirname(__file__))
CHROMIUM_SRC = os.path.abspath(os.path.join(THIS_DIR, "..", ".."))
REPROXY_CFG_PATH = os.path.join(THIS_DIR, "reproxy.cfg")

REPROXY_CFG_HEADER = """# AUTOGENERATED FILE - DO NOT EDIT
# Generated by configure_reclient_cfgs.py
# To edit:
# Update reproxy_cfg_templates/$reproxy_cfg_template
# And run 'gclient sync' or 'gclient runhooks'
"""

AUTO_AUTH_FLAGS = """
# Googler auth flags
credentials_helper={credshelper}
credentials_helper_args={args}
""".format(credshelper=os.path.join(CHROMIUM_SRC, "buildtools", "reclient",
                                    "credshelper"),
           args="--auth_source=automaticAuth --gcert_refresh_timeout=20")

ADC_AUTH_FLAGS = """
# ADC auth flags
use_application_default_credentials=true
"""

GCLOUD_AUTH_FLAGS = """
use_external_auth_token=true
credentials_helper={credshelper}
credentials_helper_args={args}
""".format(credshelper=os.path.join(CHROMIUM_SRC, "buildtools", "reclient",
                                    "credshelper"),
           args="--auth_source=gcloud")

LUCI_AUTH_CREDSHELPER_FLAGS = """
credentials_helper={credshelper}
credentials_helper_args={args}
""".format(
    credshelper=os.path.join("luci-auth"),
    args=" ".join([
        "token",
        "-scopes-context",
        "-json-output=-",
        "-json-format=reclient",
        "-lifetime=5m",
    ]),
)


def ClangRevision():
    sys.path.insert(0, os.path.join(CHROMIUM_SRC, "tools", "clang", "scripts"))
    import update
    sys.path.pop(0)
    return update.PACKAGE_VERSION


class CipdError(Exception):
    """Raised by configure_reclient_cfgs on fatal cipd error."""


class CipdAuthError(CipdError):
    """Raised by configure_reclient_cfgs on cipd auth error."""


def CipdEnsure(pkg_name, ref, directory, quiet):
    logging.info("ensure %s %s in %s", pkg_name, ref, directory)
    log_level = "warning" if quiet else "info"
    ensure_file = """
$ParanoidMode CheckIntegrity
{pkg} {ref}
""".format(pkg=pkg_name, ref=ref)
    try:
        output = subprocess.check_output(
            " ".join([
                "cipd", "ensure", "-log-level=" + log_level, "-root",
                directory, "-ensure-file", "-"
            ]),
            shell=True,
            input=ensure_file,
            stderr=subprocess.STDOUT,
            universal_newlines=True,
        )
        logging.info(output)
    except subprocess.CalledProcessError as e:
        if not IsCipdLoggedIn():
            raise CipdAuthError(e.output) from e
        raise CipdError(e.output) from e


def IsCipdLoggedIn():
    ps = subprocess.run(
        ' '.join(['cipd', 'auth-info']),
        shell=True, capture_output=True, text=True)
    logging.warning(
        "log for http://b/304677840: stdout from cipd auth-info: %s",
        ps.stdout)
    logging.warning(
        "log for http://b/304677840: stderr from cipd auth-info: %s",
        ps.stderr)
    return ps.returncode == 0


def RbeProjectFromInstance(instance):
    m = re.fullmatch(r"projects/([-\w]+)/instances/[-\w]+", instance)
    if not m:
        return None
    return m.group(1)


def GenerateReproxyCfg(reproxy_cfg_template, rbe_instance, rbe_project,
                       use_luci_auth):
    tmpl_path = os.path.join(THIS_DIR, "reproxy_cfg_templates",
                             reproxy_cfg_template)
    logging.info(f"generate reproxy.cfg using {tmpl_path}")
    if not os.path.isfile(tmpl_path):
        logging.warning(f"{tmpl_path} does not exist")
        return False
    with open(tmpl_path) as f:
        reproxy_cfg_tmpl = string.Template(REPROXY_CFG_HEADER + f.read())
    depsscanner_address = "exec://" + os.path.join(
        CHROMIUM_SRC, "buildtools", "reclient", "scandeps_server")
    auth_flags = AUTO_AUTH_FLAGS
    if sys.platform.startswith("win"):
        depsscanner_address += ".exe"
        auth_flags = ADC_AUTH_FLAGS
    if sys.platform == "darwin":
        auth_flags = GCLOUD_AUTH_FLAGS
    if use_luci_auth:
        auth_flags = LUCI_AUTH_CREDSHELPER_FLAGS

    reproxy_cfg = reproxy_cfg_tmpl.substitute({
        "rbe_instance": rbe_instance,
        "rbe_project": rbe_project,
        "reproxy_cfg_template": reproxy_cfg_template,
        "depsscanner_address": depsscanner_address,
        "auth_flags": auth_flags,
    })
    with open(REPROXY_CFG_PATH, "w") as f:
        f.write(reproxy_cfg)
    return True


def RequestCipdAuthentication():
    """Requests that the user authenticate to access CIPD packages."""

    print("Access to remoteexec config CIPD package requires authentication.")
    print("-----------------------------------------------------------------")
    print()
    print("I'm sorry for the hassle, but you may need to do a one-time manual")
    print("authentication. Please run:")
    print()
    print("    update_depot_tools && cipd auth-login")
    print()
    print("and follow the instructions.")
    print()
    print("NOTE: Use your google.com credentials, not chromium.org.")
    print()
    print("-----------------------------------------------------------------")
    print()
    sys.stdout.flush()


def ReadConfig():
    entries = {}
    if not os.path.isfile(REPROXY_CFG_PATH):
        logging.error('The reproxy.cfg file has not been generated yet')
        return entries
    with open(REPROXY_CFG_PATH, 'r') as f:
        for line in f:
            parts = line.strip().split('=')
            if len(parts) > 1:
                entries[parts[0].strip()] = parts[1].strip()
        return entries


def main():
    parser = argparse.ArgumentParser(
        description="configure reclient cfgs",
        formatter_class=argparse.RawTextHelpFormatter,
    )
    parser.add_argument(
        "--rewrapper_cfg_project",
        "--rbe_project",
        help="RBE instance project id for rewrapper configs. "
        "Only specify if different from --rbe_instance\n"
        "TODO(b/270201776) --rbe_project is deprecated, "
        "remove once no more usages exist",
    )
    parser.add_argument(
        "--reproxy_cfg_template",
        help="If set REPROXY_CFG_TEMPLATE will be used to "
        "generate reproxy.cfg. --rbe_instance must be "
        "specified.",
    )
    parser.add_argument(
        "--rbe_instance",
        help="RBE instance for rewrapper and reproxy configs",
        default=os.environ.get("RBE_instance"),
    )
    parser.add_argument(
        "--cipd_prefix",
        help="cipd package name prefix",
        default="infra_internal/rbe/reclient_cfgs",
    )
    parser.add_argument(
        "--skip_remoteexec_cfg_fetch",
        help="skip downloading reclient cfgs from CIPD server",
        action="store_true",
    )
    parser.add_argument(
        "--use_luci_auth_credshelper",
        help="use luci_auth in credshelper mode for authentication",
        action="store_true",
    )
    parser.add_argument("--quiet",
                        help="Suppresses info logs",
                        action="store_true")
    parser.add_argument("--get-rbe-instance",
                        help="Print the currently configured rbe instance to "
                        "stdout",
                        action="store_true")

    args = parser.parse_args()

    logging.basicConfig(
        level=logging.WARNING if args.quiet else logging.INFO,
        format="%(message)s",
    )

    if not args.rewrapper_cfg_project and \
       not args.rbe_instance and \
       not args.get_rbe_instance:
        logging.error(
            "At least one of --rbe_instance, --rewrapper_cfg_project or "
            "--get-rbe-instance must be provided")
        return 1

    if args.get_rbe_instance:
        config = ReadConfig()
        # This makes the assumption the $rbe_instance is saved under "instance"
        # in the original template
        if not "instance" in config:
            return 1
        logging.info(config["instance"])
        return 0

    rbe_project = args.rewrapper_cfg_project
    if not args.rewrapper_cfg_project:
        rbe_project = RbeProjectFromInstance(args.rbe_instance)

    if args.reproxy_cfg_template:
        if not args.rbe_instance:
            logging.error(
                "--rbe_instance is required if --reproxy_cfg_template is set")
            return 1
        if not GenerateReproxyCfg(args.reproxy_cfg_template, args.rbe_instance,
                                  rbe_project, args.use_luci_auth_credshelper):
            return 1

    if args.skip_remoteexec_cfg_fetch:
        return 0

    logging.info("fetch reclient_cfgs for RBE project %s..." % rbe_project)

    cipd_prefix = posixpath.join(args.cipd_prefix, rbe_project)

    # cleanup unused nacl configs. preserve nacl/rewrapper_linux.cfg
    # TODO(b/354804085): remove this code.
    for cfg in [
            "nacl/rewrapper_mac.cfg",
            "nacl/rewrapper_windows.cfg",
            "nacl/win-cross",
    ]:
        cfgpath = os.path.join(THIS_DIR, cfg)
        if os.path.exists(cfgpath):
            if os.path.isdir(cfgpath):

                def onerror(func, cfgpath, exc_info):
                    os.chmod(cfgpath, 0o644)
                    func(cfgpath)

                shutil.rmtree(cfgpath, onerror=onerror)
            else:
                os.chmod(cfgpath, 0o644)
                os.remove(cfgpath)

    tool_revisions = {
        "chromium-browser-clang": ClangRevision(),
        "python": "3.10",
    }
    for toolchain in tool_revisions:
        revision = tool_revisions[toolchain]
        if not revision:
            logging.info("failed to detect %s revision" % toolchain)
            continue

        toolchain_root = os.path.join(THIS_DIR, toolchain)
        cipd_ref = "revision/" + revision
        # 'cipd ensure' initializes the directory.
        try:
            CipdEnsure(
                posixpath.join(cipd_prefix, toolchain),
                ref=cipd_ref,
                directory=toolchain_root,
                quiet=args.quiet,
            )
        except CipdAuthError as e:
            RequestCipdAuthentication()
            return 1
        except CipdError as e:
            logging.error(e)
            return 1
        win_cross_cfg_dir = "win-cross"
        wcedir = os.path.join(THIS_DIR, win_cross_cfg_dir, toolchain)
        if not os.path.exists(wcedir):
            os.makedirs(wcedir, mode=0o755)
        if os.path.exists(os.path.join(toolchain_root, win_cross_cfg_dir)):
            # copy in win-cross/toolchain
            # as windows may not use symlinks.
            for cfg in glob.glob(
                    os.path.join(toolchain_root, win_cross_cfg_dir, "*.cfg")):
                fname = os.path.join(wcedir, os.path.basename(cfg))
                if os.path.exists(fname):
                    os.chmod(fname, 0o777)
                    os.remove(fname)
                logging.info("Copy from %s to %s..." % (cfg, fname))
                shutil.copy(cfg, fname)
    return 0


if __name__ == "__main__":
    sys.exit(main())
