// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/formats/mp4/h265_annex_b_to_hevc_bitstream_converter.h"

#include <stdint.h>

#include <memory>
#include <string>

#include "base/files/file.h"
#include "base/path_service.h"
#include "media/formats/mp4/avc.h"
#include "media/formats/mp4/box_definitions.h"
#include "media/parsers/h265_parser.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

std::vector<uint8_t> ReadTestFile(std::string name) {
  base::FilePath path;
  base::PathService::Get(base::DIR_SRC_TEST_DATA_ROOT, &path);
  path = path.Append(FILE_PATH_LITERAL(
                         "media/formats/mp4/h265_annex_b_fuzz_corpus"))
             .AppendASCII(name);
  base::File f(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  DCHECK(f.IsValid()) << "path: " << path.AsUTF8Unsafe();
  auto size = f.GetLength();

  std::vector<uint8_t> result(size);
  if (size > 0)
    f.ReadAtCurrentPosAndCheck(result);
  return result;
}

}  // namespace

namespace media {

TEST(H265AnnexBToHevcBitstreamConverterTest, Success) {
  std::string chunks[] = {"chunk1-config-idr.bin", "chunk2-non-idr.bin",
                          "chunk3-non-idr.bin",    "chunk4-non-idr.bin",
                          "chunk5-non-idr.bin",    "chunk6-config-idr.bin",
                          "chunk7-non-idr.bin"};
  for (bool add_parameter_sets_in_bitstream : {false, true}) {
    H265AnnexBToHevcBitstreamConverter converter(
        add_parameter_sets_in_bitstream);
    for (std::string& name : chunks) {
      SCOPED_TRACE(name);
      auto input = ReadTestFile(name);
      SCOPED_TRACE(input.size());
      std::vector<uint8_t> output;
      size_t desired_size = 0;
      bool config_changed = false;

      auto status =
          converter.ConvertChunk(input, output, &config_changed, &desired_size);
      ASSERT_EQ(status.code(), MP4Status::Codes::kBufferTooSmall);
      output.resize(desired_size);

      status = converter.ConvertChunk(input, output, &config_changed, nullptr);
      EXPECT_TRUE(status.is_ok()) << status.message();

      // Convert HEVC bitstream back to Annex-B bitstream, and validate if
      // parameter set are write to the output or not.
      EXPECT_TRUE(mp4::AVC::ConvertAVCToAnnexBInPlaceForLengthSize4(&output));
      H265Parser parser;
      parser.SetStream(output.data(), output.size());
      std::vector<H265NALU> parameter_sets;
      while (true) {
        H265NALU nalu;
        H265Parser::Result res = parser.AdvanceToNextNALU(&nalu);
        if (res == H265Parser::kEOStream) {
          break;
        }
        EXPECT_EQ(res, H265Parser::kOk);
        switch (nalu.nal_unit_type) {
          case H265NALU::VPS_NUT:
            int vps_id;
            EXPECT_EQ(parser.ParseVPS(&vps_id), H265Parser::kOk);
            EXPECT_TRUE(!!parser.GetVPS(vps_id));
            parameter_sets.push_back(nalu);
            break;
          case H265NALU::SPS_NUT:
            int sps_id;
            EXPECT_EQ(parser.ParseSPS(&sps_id), H265Parser::kOk);
            EXPECT_TRUE(!!parser.GetSPS(sps_id));
            parameter_sets.push_back(nalu);
            break;
          case H265NALU::PPS_NUT:
            int pps_id;
            EXPECT_EQ(parser.ParsePPS(nalu, &pps_id), H265Parser::kOk);
            EXPECT_TRUE(!!parser.GetPPS(pps_id));
            parameter_sets.push_back(nalu);
            break;
          default:
            break;
        }
      }

      if (add_parameter_sets_in_bitstream && config_changed) {
        // Parameter sets should write to the output stream in the order of VPS,
        // SPS, and PPS.
        EXPECT_EQ(parameter_sets.size(), 3u);
        EXPECT_EQ(parameter_sets.at(0).nal_unit_type, H265NALU::VPS_NUT);
        EXPECT_EQ(parameter_sets.at(1).nal_unit_type, H265NALU::SPS_NUT);
        EXPECT_EQ(parameter_sets.at(2).nal_unit_type, H265NALU::PPS_NUT);
      } else {
        // Parameter sets should not write to the output stream.
        EXPECT_EQ(parameter_sets.size(), 0u);
      }

      auto& config = converter.GetCurrentConfig();
      if (name.find("config") != std::string::npos) {
        // Chunks with configuration
        EXPECT_TRUE(config_changed);

        EXPECT_EQ(config.configurationVersion, 1);
        EXPECT_EQ(config.general_profile_space, 0);
        EXPECT_EQ(config.general_tier_flag, 0);
        EXPECT_EQ(config.general_profile_idc, 1);
        EXPECT_EQ(config.general_profile_compatibility_flags, 0x60000000ul);
        EXPECT_EQ(config.general_constraint_indicator_flags, 0x800000000000ull);
        EXPECT_EQ(config.general_level_idc, 60);
        EXPECT_EQ(config.min_spatial_segmentation_idc, 0);
        EXPECT_EQ(config.parallelismType, 0);
        EXPECT_EQ(config.chromaFormat, 1);
        EXPECT_EQ(config.bitDepthLumaMinus8, 0);
        EXPECT_EQ(config.bitDepthChromaMinus8, 0);
        EXPECT_EQ(config.avgFrameRate, 0);
        EXPECT_EQ(config.numOfArrays, 3);
        EXPECT_EQ(config.arrays[0].first_byte,
                  add_parameter_sets_in_bitstream ? 32 : 160);
        EXPECT_EQ(config.arrays[0].units.size(), 1ul);
        EXPECT_EQ(config.arrays[1].first_byte,
                  add_parameter_sets_in_bitstream ? 33 : 161);
        EXPECT_EQ(config.arrays[1].units.size(), 1ul);
        EXPECT_EQ(config.arrays[2].first_byte,
                  add_parameter_sets_in_bitstream ? 34 : 162);
        EXPECT_EQ(config.arrays[2].units.size(), 1ul);
      } else {
        EXPECT_FALSE(config_changed);
      }

      std::vector<uint8_t> config_bin;
      EXPECT_TRUE(config.Serialize(config_bin)) << " file: " << name;
    }
  }
}

// Tests that stream can contain multiple picture parameter sets and switch
// between them without having to reconfigure the decoder.
TEST(H265AnnexBToHevcBitstreamConverterTest, PPS_SwitchWithoutReconfig) {
  std::vector<uint8_t> vps{0x00, 0x00, 0x00, 0x01, 0x40, 0x01, 0x0c,
                           0x01, 0xff, 0xff, 0x22, 0x20, 0x00, 0x00,
                           0x03, 0x00, 0x90, 0x00, 0x00, 0x03, 0x00,
                           0x00, 0x03, 0x00, 0x99, 0x2c, 0x09};
  std::vector<uint8_t> sps{
      0x00, 0x00, 0x00, 0x01, 0x42, 0x01, 0x01, 0x22, 0x20, 0x00, 0x00, 0x03,
      0x00, 0x90, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x99, 0xa0, 0x01,
      0xe0, 0x20, 0x02, 0x1c, 0x4d, 0x94, 0xbb, 0xb4, 0xa3, 0x32, 0xaa, 0xc0,
      0x5a, 0x84, 0x89, 0x04, 0x8a, 0x00, 0x00, 0x07, 0xd0, 0x00, 0x01, 0x86,
      0xa0, 0xe4, 0x68, 0x7c, 0x95, 0x00, 0x00, 0x89, 0x54, 0x00, 0x00, 0xf7,
      0x31, 0x00, 0x00, 0x44, 0xaa, 0x00, 0x00, 0x7b, 0x98, 0x88};
  std::vector<uint8_t> pps1{
      0x00, 0x00, 0x00, 0x01, 0x44, 0x01, 0xc0, 0x72, 0xb0, 0x3b, 0xc4, 0x0c,
      0x88, 0xc6, 0x70, 0x86, 0x18, 0x82, 0x08, 0x80, 0xc4, 0x10, 0x60, 0xa3,
      0x81, 0x23, 0x02, 0x06, 0x4c, 0x7f, 0xff, 0xf2, 0x88, 0x11, 0x26, 0x4e,
      0x4f, 0x27, 0xc4, 0x7e, 0x23, 0xf8, 0x8f, 0xc6, 0x7c, 0x67, 0x84, 0x38,
      0x43, 0xf1, 0x03, 0x23, 0x30, 0x87, 0x08, 0x78, 0x43, 0xe1, 0x8f, 0xc1,
      0x07, 0xf0, 0x51, 0xf8, 0x10, 0x3f, 0xff, 0xfc, 0xa3, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xc7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0x28, 0x82, 0x12, 0x4c, 0x9c, 0x9e, 0x4f, 0x88, 0xfc, 0x47,
      0xf1, 0x1f, 0x8c, 0xf8, 0xcf, 0x08, 0x70, 0x87, 0xd6, 0x3f, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xfd, 0x55, 0x20};
  std::vector<uint8_t> pps2{
      0x00, 0x00, 0x00, 0x01, 0x44, 0x01, 0x50, 0x1c, 0xac, 0x0e, 0xf1, 0x03,
      0x22, 0x31, 0x9c, 0x21, 0x86, 0x20, 0x82, 0x20, 0x31, 0x04, 0x18, 0x28,
      0xe0, 0x48, 0xc0, 0x81, 0x93, 0x0a, 0x07, 0x20, 0x93, 0x07, 0x9c, 0x0c,
      0x1f, 0x28, 0x81, 0x12, 0x64, 0xe4, 0xf2, 0x7c, 0x47, 0xe2, 0x3f, 0x88,
      0xfc, 0x67, 0xc6, 0x78, 0x43, 0x84, 0x3f, 0x10, 0x32, 0x33, 0x08, 0x70,
      0x87, 0x84, 0x3e, 0x18, 0xfc, 0x10, 0x7f, 0x05, 0x1f, 0x81, 0x03, 0xff,
      0xff, 0xca, 0x14, 0x11, 0x26, 0x4e, 0x4f, 0x11, 0xf1, 0x1f, 0x88, 0xfe,
      0x33, 0xf0, 0x87, 0xc2, 0x9f, 0xff, 0xc2, 0x81, 0x08, 0x43, 0x0a, 0x70,
      0xc7, 0x85, 0x04, 0x78, 0x20, 0xfc, 0x10, 0x7f, 0x06, 0x9f, 0xff, 0xff,
      0xfc, 0xa2, 0x08, 0x49, 0x32, 0x72, 0x79, 0x3e, 0x23, 0xf1, 0x1f, 0xc4,
      0x7e, 0x33, 0xe3, 0x3c, 0x21, 0xc2, 0x1f, 0x58, 0x50, 0x41, 0x22, 0x31,
      0x1c, 0x67, 0x8c, 0xf8, 0xcf, 0xc6, 0x7f, 0x08, 0x7f, 0xff, 0xff, 0xf5,
      0x54, 0x80};
  std::vector<uint8_t> first_frame_idr{
      0x00, 0x00, 0x00, 0x01, 0x28, 0x01, 0xac, 0x6d, 0xa0,
      0x7c, 0x96, 0x84, 0xdb, 0xcc, 0xf7, 0x4f, 0x9d, 0xf4,
      0x94, 0x85, 0x37, 0x06, 0x66, 0xf8
      // Encoded data omitted here, it's not important for NALU parsing
  };

  std::vector<uint8_t> first_chunk;
  first_chunk.insert(first_chunk.end(), vps.begin(), vps.end());
  first_chunk.insert(first_chunk.end(), sps.begin(), sps.end());
  first_chunk.insert(first_chunk.end(), pps1.begin(), pps1.end());
  first_chunk.insert(first_chunk.end(), pps2.begin(), pps2.end());
  first_chunk.insert(first_chunk.end(), first_frame_idr.begin(),
                     first_frame_idr.end());

  std::vector<uint8_t> second_non_idr_chunk{
      0x00, 0x00, 0x00, 0x01, 0x02, 0x01, 0xa4, 0x04, 0x55, 0xa2, 0x6d, 0xc3,
      0xc0, 0xc3, 0x3d, 0x0b, 0xac, 0xbc, 0x00, 0xc4, 0x44, 0x2e, 0xf7, 0x55,
      0xfd, 0x05, 0x86, 0x92, 0x19, 0xdf, 0x58, 0xec, 0x38, 0x36, 0xb7, 0x7c,
      0x00, 0x15, 0x33, 0x78, 0x03, 0x67, 0x26, 0x0f
      // Encoded data omitted here, it's not important for NALU parsing
  };

  for (bool add_parameter_sets_in_bitstream : {false, true}) {
    H265AnnexBToHevcBitstreamConverter converter(
        add_parameter_sets_in_bitstream);
    std::vector<uint8_t> output(10000);
    bool config_changed = false;

    auto status =
        converter.ConvertChunk(first_chunk, output, &config_changed, nullptr);
    EXPECT_TRUE(status.is_ok()) << status.message();
    EXPECT_TRUE(config_changed);

    // Convert HEVC bitstream back to Annex-B bitstream, and validate if
    // parameter set are write to the output.
    EXPECT_FALSE(mp4::AVC::ConvertAVCToAnnexBInPlaceForLengthSize4(&output));
    H265Parser parser;
    parser.SetStream(output.data(), output.size());
    std::vector<H265NALU> parameter_sets;
    while (true) {
      H265NALU nalu;
      H265Parser::Result res = parser.AdvanceToNextNALU(&nalu);
      if (res == H265Parser::kEOStream) {
        break;
      }
      EXPECT_EQ(res, H265Parser::kOk);
      switch (nalu.nal_unit_type) {
        case H265NALU::VPS_NUT:
          int vps_id;
          EXPECT_EQ(parser.ParseVPS(&vps_id), H265Parser::kOk);
          EXPECT_TRUE(!!parser.GetVPS(vps_id));
          parameter_sets.push_back(nalu);
          break;
        case H265NALU::SPS_NUT:
          int sps_id;
          EXPECT_EQ(parser.ParseSPS(&sps_id), H265Parser::kOk);
          EXPECT_TRUE(!!parser.GetSPS(sps_id));
          parameter_sets.push_back(nalu);
          break;
        case H265NALU::PPS_NUT:
          int pps_id;
          EXPECT_EQ(parser.ParsePPS(nalu, &pps_id), H265Parser::kOk);
          EXPECT_TRUE(!!parser.GetPPS(pps_id));
          parameter_sets.push_back(nalu);
          break;
        default:
          break;
      }
    }

    if (add_parameter_sets_in_bitstream) {
      // Parameter sets should write to the output stream in the order of VPS,
      // SPS, PPS1, and PPS2.
      EXPECT_EQ(parameter_sets.size(), 4u);
      EXPECT_EQ(parameter_sets.at(0).nal_unit_type, H265NALU::VPS_NUT);
      EXPECT_EQ(parameter_sets.at(1).nal_unit_type, H265NALU::SPS_NUT);
      EXPECT_EQ(parameter_sets.at(2).nal_unit_type, H265NALU::PPS_NUT);
      EXPECT_EQ(parameter_sets.at(3).nal_unit_type, H265NALU::PPS_NUT);
    } else {
      // Parameter sets should not write to the output stream.
      EXPECT_EQ(parameter_sets.size(), 0u);
    }

    auto& config = converter.GetCurrentConfig();
    EXPECT_EQ(config.numOfArrays, 3);
    EXPECT_EQ(config.arrays[0].first_byte,
              add_parameter_sets_in_bitstream ? 32 : 160);
    EXPECT_EQ(config.arrays[0].units.size(), 1ul);
    EXPECT_EQ(config.arrays[1].first_byte,
              add_parameter_sets_in_bitstream ? 33 : 161);
    EXPECT_EQ(config.arrays[1].units.size(), 1ul);
    EXPECT_EQ(config.arrays[2].first_byte,
              add_parameter_sets_in_bitstream ? 34 : 162);
    EXPECT_EQ(config.arrays[2].units.size(), 2ul);
    status = converter.ConvertChunk(second_non_idr_chunk, output,
                                    &config_changed, nullptr);
    EXPECT_TRUE(status.is_ok()) << status.message();
    EXPECT_FALSE(config_changed);
  }
}

TEST(H265AnnexBToHevcBitstreamConverterTest, Failure) {
  for (bool add_parameter_sets_in_bitstream : {false, true}) {
    H265AnnexBToHevcBitstreamConverter converter(
        add_parameter_sets_in_bitstream);

    std::vector<uint8_t> input{0x0,  0x0, 0x0, 0x1, 0x40, 0x01,
                               0x0c, 0x0, 0x0, 0x1, 0x67, 0x42};
    std::vector<uint8_t> output(input.size());

    auto status = converter.ConvertChunk(input, output, nullptr, nullptr);

    ASSERT_EQ(status.code(), MP4Status::Codes::kInvalidVPS);
  }
}

}  // namespace media
