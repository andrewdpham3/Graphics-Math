  ///////////////////////////////////////////////////////////////////////////////
  // gfxfilter.hh
  //
  // Raster image filters. This module defines image filters for:
  //
  //     - clear one of the three RGB components;
  //     - scale one of the components;
  //     - crop;
  //     - extend edges;
  //     - crop extended edges;
  //     - convert color to grayscale;
  //     - Sobel edge detection; and
  //     - box blur.
  //
  // This module builds on gfximage.hh, so familiarize yourself with
  // that file before using this one.
  //
  ///////////////////////////////////////////////////////////////////////////////

  #pragma once

  #include <algorithm>
  #include <cmath>
  #include <iostream>
  #include "gfximage.hh"
  using namespace std;

  namespace gfx {

    // Clear one color component. Make after contain a copy of before,
    // except that every component_to_clear intensity has been set to
    // zero. For example if component_to_clear is gfx::RGB_INDEX_RED,
    // then every red component in after is zero, while the green and
    // blue components are copied over from before unchanged. before
    // must be non-empty, and component_to_clear must be a valid rgb
    // index.
    template <typename color_depth>
    void clear_component(gfx::image<color_depth>& after,
  		       const gfx::image<color_depth>& before,
  		       rgb_index component_to_clear) {

      // Check arguments.
      assert(!before.empty());
      assert(is_rgb_index(component_to_clear));

      // Resize after to the necessary dimensions.
      after.same_size(before);

      // Loop through all pixel coordinates.
      for (int y = 0; y < before.height(); ++y) {
        for (int x = 0; x < before.width(); ++x) {

  	// Make a copy of the "before" pixel color.
  	gfx::rgb<color_depth> pixel = before.pixel(x, y);

  	// Clear the desired component.
  	pixel[component_to_clear] = 0;

  	// Copy the resulting pixel into the "after" image object.
  	after.pixel(x, y) = pixel;
        }
      }
    }

    // Scale one color component. Make after contain a copy of before,
    // except that every component_to_clear intensity has been
    // multiplied by scale_factor. For example if component_to_clear is
    // gfx::RGB_INDEX_RED and scale_factor is 1.5, then every red
    // component is increased 150%. The resulting intensity values are
    // clamped into the range [0, color_depth::max_value]. before must
    // be non-empty, component_to_scale must be a valid rgb index, and
    // scale_factor must be non-negative.
    template <typename color_depth>
    void scale_component(gfx::image<color_depth>& after,
  		       const gfx::image<color_depth>& before,
  		       rgb_index component_to_scale,
  		       double scale_factor) {

      // Check arguments.
      assert(!before.empty());
      assert(is_rgb_index(component_to_scale));
      assert(scale_factor >= 0.0);

      // Resize after to the necessary dimensions.
      after.same_size(before);

      // Loop through all pixel coordinates.
      for (int y = 0; y < before.height(); ++y) {
        for (int x = 0; x < before.width(); ++x) {

  	// Get a copy of the original pixel.
  	gfx::rgb<color_depth> original_pixel = before.pixel(x, y);

  	// Convert that pixel to HDR, so that we can do the scale
  	// arithmetic using floating-point data types. Note the
  	// peculiar syntax needed to instantiate the convert_to
  	// template function.
  	gfx::hdr_rgb hdr_pixel = original_pixel.template convert_to<hdr_color_depth>();

  	// first do the scale multiplication...
  	float scaled = hdr_pixel[component_to_scale] * scale_factor,
  	  // then make sure it does not exceed the max value.
  	  clamped = std::min(scaled, 1.0f);
  	assert(clamped >= 0.0);
  	assert(clamped <= 1.0);

  	// Overwrite the un-scaled intensity with the scaled one.
  	hdr_pixel[component_to_scale] = clamped;

  	// Convert this HDR pixel to match the desired output color
  	// depth.
  	gfx::rgb<color_depth> result_pixel = hdr_pixel.convert_to<color_depth>();

  	// Write the resulting pixel to the "after" image.
  	after.pixel(x, y) = result_pixel;
        }
      }
    }

    // Crop. Make after contain the pixels from the rectangular region
    // of before, with the specified top-left corner, width, and
    // height. before must be non-empty, width and height must both be
    // positive, and the entire rectangle must fit inside before.
    template <typename color_depth>
    void crop(gfx::image<color_depth>& after,
  	    const gfx::image<color_depth>& before,
  	    int left,
  	    int top,
  	    int width,
  	    int height) {

      // Check arguments.
      assert(!before.empty());
      assert(before.is_x(left));
      assert(before.is_y(top));
      assert(width > 0);
      assert(height > 0);
      assert(before.is_x(left + width - 1));
      assert(before.is_y(top + height - 1));

      after.resize(width, height, BLACK.convert_to<color_depth>());

      for(int i=0;i<height;i++)
        for(int j=0;j<width;j++)
          after.pixel(j,i)=before.pixel(left+j,top+i);

    }

    // Extend the edges of an image. This is intended to be used as a
    // preprocessing step in a convolution filter, to create a "buffer"
    // of similar pixels around the true input image. The layout of
    // "after" is
    //
    //     +-+-------+-+
    //     +A+   B   +C+
    //     +-+-------+-+
    //     | |       | |
    //     |D|   E   |F|
    //     | |       | |
    //     +-+-------+-+
    //     +G+   H   +I+
    //     +-+-------+-+
    //
    // where:
    //    - E is a copy of the "before" image
    //    - B is a rectangle of height pad_radius, formed by copying the
    //      top row of "before"
    //    - H is, likewise, a rectangle of height pad_radius, formed by
    //      copying the bottom row of "before"
    //    - D is a rectangle of width pad_radius, formed by copying the
    //      leftmost column of "before"
    //    - F is, likewise, a rectangle of width pad_radius, formed by
    //      copying the rightmost column of "before"
    //    - A is a square of width and height pad_radius, formed by
    //      copying the top-left pixel of "before"
    //    - C is likewise a square formed by copying the top-right pixel
    //      of "before"
    //    - G is likewise a square formed by copying the bottom-left pixel
    //      of "before"
    //    - I is likewise a square formed by copying the bottom-right pixel
    //      of "before"
    //
    // Therefore the dimensions of the after image obey
    //
    //    after.width()  == before.width()  + 2 * pad_radius
    //    after.height() == before.height() + 2 * pad_radius
    //
    // before must be non-empty, and pad_radius must be positive.
    template <typename color_depth>
    void extend_edges(gfx::image<color_depth>& after,
  		    const gfx::image<color_depth>& before,
  		    int pad_radius) {

      // Check arguments.
      assert(!before.empty());
      assert(pad_radius > 0);

      // TODO: replace this function body with working code. Make sure
      // to delete this comment.

      int endwidth=before.width()+2*pad_radius;
      int endheight=before.height()+2*pad_radius;

      after.resize(endwidth, endheight);
      after.fill(BLACK.convert_to<color_depth>());

      /*check size
      cout<<endl<<pad_radius<<endl;
      cout<<endl<<before.width()<<" "<<before.height()<<endl;
      cout<<endl<<after.width()<<" "<<after.height()<<endl;
      */

      for(int i=0;i<before.height();i++)
        for(int j=0;j<before.width();j++)
          after.pixel(j+pad_radius,i+pad_radius)=before.pixel(j,i);


      //after.fill(BLUE.convert_to<color_depth>());

      // Hint: I can think of two sensible ways of implementing this
      // function. (1) write nine separate loops, one for each of the
      // regions A-I. This is tedious but relatively foolproof. (2)
      // Write one loop that iterates through every possible "after"
      // coordinate. For each, determine which of the regions A-I to
      // copy out of.

    }

    // Crop away the padding created by extend_edges. If before was
    // created with extend_edges, then after will be filled with the
    // original image labeled E in the description for extend_edges. In
    // other words, crop_extended_edges un-does extend_edges. before
    // must be non-empty and pad_radius must be positive.
    template <typename color_depth>
    void crop_extended_edges(gfx::image<color_depth>& after,
  			   const gfx::image<color_depth>& before,
  			   int pad_radius) {

      // Check arguments.
      assert(!before.empty());
      assert(pad_radius > 0);

      crop(after,before,pad_radius,pad_radius,before.width()-2*pad_radius,before.height()-2*pad_radius);
    }

    // Convert from color to grayscale. after is filled with a version
    // of before, where each rgb is converted into a grayscale (aka
    // semitone) with approximately the same perceived luminance as the
    // source pixel. before must be non-empty.
    template <typename color_depth>
    void grayscale(gfx::image<color_depth>& after,
  		 const gfx::image<color_depth>& before) {

      // Check arguments.
      assert(!before.empty());

      after=before;
      for(int i=0;i<before.height();i++)
        for(int j=0;j<before.width();j++){
          after.pixel(j,i).red()=after.pixel(j,i).red()*0.2;
          after.pixel(j,i).green()=after.pixel(j,i).green()*0.7;
          after.pixel(j,i).blue()=after.pixel(j,i).blue()*0.1;
        }
    }

    // Edge detection. Specifically, convert "before" to grayscale,
    // apply the Sobel edge detection convolution filter, and store the
    // result in "after". before must be non-empty.
    template <typename color_depth>
    void edge_detect(gfx::image<color_depth>& after,
  		   const gfx::image<color_depth>& before) {

      // Check arguments.
      assert(!before.empty());

      // TODO: replace this function body with working code. Make sure
      // to delete this comment.

      // Hint: Use the grayscale(...) and extend_edges(...) filters to
      // prepare for the Sobel convolution. Then compute the Sobel
      // operator one pixel at a time. Finally use crop_extended_edges
      // to un-do the earlier extend_edges.

    }

    // Box blur. Use the box convolution filter, with the given radius,
    // to achieve a blur effect. before must be non-empty and radius
    // must be positive.
    template <typename color_depth>
    void box_blur(gfx::image<color_depth>& after,
  		const gfx::image<color_depth>& before,
  		int radius) {

      // Check arguments.
      assert(!before.empty());
      assert(radius > 0);

      // Hint: This is a straighforward application of a convolution
      // filter. The box filter is defined in section 9.3.1 of the
      // textbook. Section 9.4 of the textbook explains how to implement
      // a convolution filter in detail.

      // Another hint: Evaluating the box filter will involve a lot of
      // rounding issues when the color intensities are int types. I
      // found it easier to first convert the source image to an
      // hdr_image, then compute the box filter entirely using floating
      // point calculations, then finally convert the resulting image
      // back to the original color_depth.

    }

  }
