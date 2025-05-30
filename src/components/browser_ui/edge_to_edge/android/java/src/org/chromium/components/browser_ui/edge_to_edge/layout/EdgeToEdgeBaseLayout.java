// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.browser_ui.edge_to_edge.layout;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff.Mode;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.widget.FrameLayout;

import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.graphics.Insets;

import org.chromium.ui.util.WindowInsetsUtils;

/**
 * A wrapper layout that adjusts padding based on the current window insets. It ensures it's child
 * views have no overlap with system insets, making them edge to edge compatible.
 *
 * <p>The layout paints the status / navigation bar color according to settings; the layout paints
 * the display cutout black. When display cutout overlaps with the status bar, the display cutout
 * paint will be ignored.
 *
 * <p>This layout is meant to be used when the activity is drawing under the system insets.
 */
public class EdgeToEdgeBaseLayout extends FrameLayout {
    private static final int DEFAULT_NAV_BAR_DIVIDER_SIZE = 1;
    private static final int DISPLAY_CUTOUT_PAINT_COLOR = Color.BLACK;

    private final Rect mViewRect = new Rect();
    private final Rect mStatusBarRect = new Rect();
    private final Rect mNavBarRect = new Rect();
    private final Rect mNavBarDividerRect = new Rect();
    // Rects used for display cutout.
    private final Rect mCutoutRectLeft = new Rect();
    private final Rect mCutoutRectRight = new Rect();
    private final Paint mStatusBarPaint = new Paint();
    private final Paint mNavBarPaint = new Paint();
    private final Paint mNavBarDividerPaint = new Paint();
    private final Paint mDisplayCutoutPaint = new Paint();

    private Insets mStatusBarInsets = Insets.NONE;
    private Insets mNavigationBarInsets = Insets.NONE;
    private Insets mCutoutInsetsLeft = Insets.NONE;
    private Insets mCutoutInsetsRight = Insets.NONE;

    public EdgeToEdgeBaseLayout(@NonNull Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);

        // Nav bar can draw on top of the status bar in landscape using 3-button mode.
        mNavBarPaint.setXfermode(new PorterDuffXfermode(Mode.SRC_OVER));

        mDisplayCutoutPaint.setColor(DISPLAY_CUTOUT_PAINT_COLOR);
    }

    @Override
    public void onDraw(@NonNull Canvas canvas) {
        // Draw colors over its padding.
        colorRectOnDraw(canvas, mStatusBarRect, mStatusBarPaint);
        colorRectOnDraw(canvas, mNavBarRect, mNavBarPaint);
        colorRectOnDraw(canvas, mCutoutRectLeft, mDisplayCutoutPaint);
        colorRectOnDraw(canvas, mCutoutRectRight, mDisplayCutoutPaint);
        if (!mNavBarDividerRect.isEmpty() && mNavBarDividerPaint.getAlpha() > 0) {
            colorRectOnDraw(canvas, mNavBarDividerRect, mNavBarDividerPaint);
        }

        super.onDraw(canvas);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        updateCachedRects();
    }

    void updateCachedRects() {
        mViewRect.set(getLeft(), getTop(), getRight(), getBottom());
        mStatusBarRect.set(WindowInsetsUtils.toRectInWindow(mViewRect, mStatusBarInsets));
        mNavBarRect.set(WindowInsetsUtils.toRectInWindow(mViewRect, mNavigationBarInsets));

        // In landscape mode, status bar can intersect with nav bar.
        if (Rect.intersects(mStatusBarRect, mNavBarRect)) {
            mStatusBarRect.left += mNavigationBarInsets.left;
            mStatusBarRect.right -= mNavigationBarInsets.right;
        }

        // When display cutout rect is coming from different direction as the status bar and
        // navigation bar, intersect those system bars.
        mCutoutRectLeft.set(WindowInsetsUtils.toRectInWindow(mViewRect, mCutoutInsetsLeft));
        mCutoutRectRight.set(WindowInsetsUtils.toRectInWindow(mViewRect, mCutoutInsetsRight));
        if (!mCutoutRectLeft.isEmpty() || !mCutoutRectRight.isEmpty()) {
            if (Rect.intersects(mStatusBarRect, mCutoutRectLeft)) {
                mStatusBarRect.left += mCutoutInsetsLeft.left;
            }
            if (Rect.intersects(mStatusBarRect, mCutoutRectRight)) {
                mStatusBarRect.right -= mCutoutInsetsRight.right;
            }
            if (Rect.intersects(mNavBarRect, mCutoutRectLeft)) {
                mNavBarRect.left += mCutoutInsetsLeft.left;
            }
            if (Rect.intersects(mNavBarRect, mCutoutRectRight)) {
                mNavBarRect.right -= mCutoutInsetsRight.right;
            }
        }

        // Set the nav bar divider the last after the nav bar adjustments.
        mNavBarDividerRect.set(getNavBarDividerRectFromInset(mNavigationBarInsets, mNavBarRect));
    }

    void setStatusBarInsets(@NonNull Insets insets) {
        mStatusBarInsets = insets;
    }

    void setNavigationBarInsets(@NonNull Insets insets) {
        mNavigationBarInsets = insets;
    }

    void setDisplayCutoutInsetLeft(@NonNull Insets insets) {
        assert insets.left > 0 || Insets.NONE.equals(insets);
        mCutoutInsetsLeft = insets;
    }

    void setDisplayCutoutInsetRight(@NonNull Insets insets) {
        assert insets.right > 0 || Insets.NONE.equals(insets);
        mCutoutInsetsRight = insets;
    }

    void setStatusBarColor(@ColorInt int color) {
        mStatusBarPaint.setColor(color);
    }

    void setNavBarColor(@ColorInt int color) {
        mNavBarPaint.setColor(color);
    }

    void setNavBarDividerColor(@ColorInt int color) {
        mNavBarDividerPaint.setColor(color);
    }

    private static void colorRectOnDraw(Canvas canvas, Rect rect, Paint paint) {
        if (rect.isEmpty()) return;
        canvas.save();
        canvas.clipRect(rect);
        canvas.drawPaint(paint);
        canvas.restore();
    }

    private static Rect getNavBarDividerRectFromInset(Insets navBarInsets, Rect navBarRect) {
        Rect navBarDivider = new Rect(navBarRect);
        if (navBarInsets.bottom > 0) { // Divider on the top edge of nav bar.
            navBarDivider.bottom = navBarDivider.top + DEFAULT_NAV_BAR_DIVIDER_SIZE;
        } else if (navBarInsets.left > 0) { // // Divider on the right edge of nav bar.
            navBarDivider.left = navBarDivider.right - DEFAULT_NAV_BAR_DIVIDER_SIZE;
        } else if (navBarInsets.right > 0) { // Divider on the left edge of nav bar.
            navBarDivider.right = navBarDivider.left + DEFAULT_NAV_BAR_DIVIDER_SIZE;
        } else {
            assert navBarInsets.top <= 0 : "Nav bar insets should not be at the top.";
        }

        return navBarDivider;
    }

    Rect getStatusBarRectForTesting() {
        return mStatusBarRect;
    }

    Rect getNavigationBarRectForTesting() {
        return mNavBarRect;
    }

    Rect getNavigationBarDividerRectForTesting() {
        return mNavBarDividerRect;
    }

    Rect getCutoutRectLeftForTesting() {
        return mCutoutRectLeft;
    }

    Rect getCutoutRectRightForTesting() {
        return mCutoutRectRight;
    }
}
