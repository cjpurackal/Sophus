// Copyright (c) 2011, Hauke Strasdat
// Copyright (c) 2012, Steven Lovegrove
// Copyright (c) 2021, farm-ng, inc.
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once

#include "sophus/image/runtime_image_view.h"

#include <variant>

namespace sophus {

/// Type-erased image with shared ownership, and read-only access to pixels.
/// Type is nullable.
template <
    class TPredicate = AnyImagePredicate,
    template <class> class TAllocator = Eigen::aligned_allocator>
class RuntimeImage : public RuntimeImageView<TPredicate> {
 public:
  /// Empty image.
  RuntimeImage() = default;

  /// Create type-erased image from Image.
  ///
  /// Ownership is shared between RuntimeImage and Image, and hence the
  /// reference count will be increased by one (unless input is empty).
  /// By design not "explicit".
  template <class TPixel>
  RuntimeImage(Image<TPixel, TAllocator> const& image)
      : RuntimeImage(
            image.shape(),
            RuntimePixelType::fromTemplate<TPixel>(),
            image.shared_) {
    static_assert(TPredicate::template isTypeValid<TPixel>());
  }

  /// Create type-erased image from MutImage.
  /// By design not "explicit".
  template <class TPixel>
  RuntimeImage(MutImage<TPixel>&& image)
      : RuntimeImage(Image<TPixel>(std::move(image))) {
    static_assert(TPredicate::template isTypeValid<TPixel>());
  }

  /// Create type-image image from provided shape and pixel type.
  /// Pixel data is left uninitialized
  RuntimeImage(ImageShape const& shape, RuntimePixelType const& pixel_type)
      : RuntimeImage(
            shape,
            pixel_type,
            std::shared_ptr<uint8_t>(TAllocator<uint8_t>().allocate(
                shape.height() * shape.pitchBytes()))) {
    // TODO: Missing check on ImagePredicate against pixel_type
    //       has to be a runtime check, since we don't know at runtime.

    SOPHUS_ASSERT_LE(
        shape.width() * pixel_type.num_channels *
            pixel_type.num_bytes_per_pixel_channel,
        (int)shape.pitchBytes());
  }

  /// Create type-image image from provided size and pixel type.
  /// Pixel data is left uninitialized
  RuntimeImage(ImageSize const& size, RuntimePixelType const& pixel_type)
      : RuntimeImage(
            ImageShape::makeFromSizeAndPitch<uint8_t>(
                size,
                size.width * pixel_type.num_channels *
                    pixel_type.num_bytes_per_pixel_channel),
            pixel_type) {}

  template <class TT>
  static RuntimeImage makeCopyFrom(ImageView<TT> image_view) {
    return Image<TT>::makeCopyFrom(image_view);
  }

  /// Return true is this contains data of type TPixel.
  template <class TPixel>
  [[nodiscard]] bool has() const noexcept {
    RuntimePixelType expected_type = RuntimePixelType::fromTemplate<TPixel>();
    return expected_type == this->pixel_type_;
  }

  /// Returns typed image.
  ///
  /// Precondition: this->has<TPixel>()
  template <class TPixel>
  [[nodiscard]] Image<TPixel, TAllocator> image() const noexcept {
    if (!this->has<TPixel>()) {
      RuntimePixelType expected_type = RuntimePixelType::fromTemplate<TPixel>();

      SOPHUS_PANIC(
          "expected type: {}\n"
          "actual type: {}",
          expected_type,
          this->pixel_type_);
    }

    return Image<TPixel, TAllocator>(
        ImageView<TPixel>(
            this->shape_, reinterpret_cast<TPixel*>(shared_.get())),
        shared_);
  }

  template <class TPixel>
  Image<TPixel, TAllocator> reinterpretAs(
      ImageSize reinterpreted_size) const noexcept {
    SOPHUS_ASSERT_LE(
        reinterpreted_size.width * sizeof(TPixel), this->shape().pitch_bytes_);
    SOPHUS_ASSERT_LE(reinterpreted_size.height, this->height());

    SOPHUS_UNIMPLEMENTED();
  }

  [[nodiscard]] size_t useCount() const { return shared_.use_count(); }

 private:
  // Private constructor mainly available for constructing sub-views
  RuntimeImage(
      ImageShape shape,
      RuntimePixelType pixel_type,
      std::shared_ptr<uint8_t> shared)
      : RuntimeImageView<TPredicate>(shape, pixel_type, shared.get()),
        shared_(shared) {}

  std::shared_ptr<uint8_t> shared_;
};

}  // namespace sophus