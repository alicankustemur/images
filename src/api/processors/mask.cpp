#include "processors/mask.h"

namespace weserv {
namespace api {
namespace processors {

using namespace enums;

std::string Mask::svg_path_by_type(const int width, const int height,
                                   const MaskType &mask,
                                   int *out_x_min, int *out_y_min,
                                   int *out_width, int *out_height) const {
    int min = std::min(width, height);
    float outer_radius = static_cast<float>(min) / 2.0F;
    float mid_x = static_cast<float>(width) / 2.0F;
    float mid_y = static_cast<float>(height) / 2.0F;

    if (mask == MaskType::Heart) {
        std::vector<PathCoordinate> coordinates =
            heart_path(outer_radius, outer_radius, out_x_min, out_y_min,
                       out_width, out_height);

        PathCoordinate mask_transl{};
        double scale;
        std::tie(mask_transl, scale) = translation_and_scaling(
            width, height, *out_x_min, *out_y_min, out_width, out_height);

        return transformed_path_string(coordinates, mask_transl, scale);
    }

    if (mask == MaskType::Ellipse) {
        *out_x_min = 0;
        *out_y_min = 0;
        *out_width = width;
        *out_height = height;

        return svg_ellipse_path(mid_x, mid_y, mid_x, mid_y);
    }

    if (mask == MaskType::Circle) {
        *out_x_min = static_cast<int>(std::round(mid_x - outer_radius));
        *out_y_min = static_cast<int>(std::round(mid_y - outer_radius));
        *out_width = min;
        *out_height = min;

        return svg_circle_path(mid_x, mid_y, outer_radius);
    }

    // 'inner' radius of the polygon/star
    float inner_radius = outer_radius;

    // Initial angle (clockwise). By default, stars and polygons are 'pointing'
    // up.
    double initial_angle = 0.0;

    // Number of points (or number of sides for polygons)
    int points = 0;

    switch (mask) {
        case MaskType::Hexagon:
            // Hexagon
            points = 6;
            break;
        case MaskType::Pentagon:
            // Pentagon
            points = 5;
            break;
        case MaskType::Pentagon180:
            // Pentagon tilted upside down
            points = 5;
            initial_angle = M_PI;
            break;
        case MaskType::Star:
            // 5 point star
            points = 5 * 2;
            inner_radius *= 0.382F;
            break;
        case MaskType::Square:
            // Square tilted 45 degrees
            points = 4;
            break;
        case MaskType::Triangle:
            // Triangle
            points = 3;
            break;
        case MaskType::Triangle180:
            // Triangle upside down
            points = 3;
            initial_angle = M_PI;
            break;
        default:  // LCOV_EXCL_START
            throw std::logic_error("Reached a supposed unreachable point");
            // LCOV_EXCL_STOP
    }

    std::vector<PathCoordinate> coordinates;

    float x_max = std::numeric_limits<float>::min();
    float y_max = std::numeric_limits<float>::min();
    float x_min = std::numeric_limits<float>::max();
    float y_min = std::numeric_limits<float>::max();

    for (int i = 0; i < points; ++i) {
        double angle = i * 2 * M_PI / points - M_PI / 2 + initial_angle;
        float radius = inner_radius;
        if (i % 2 == 0) {
            radius = outer_radius;
        }

        auto x = static_cast<float>(mid_x + radius * std::cos(angle));
        auto y = static_cast<float>(mid_y + radius * std::sin(angle));

        coordinates.push_back({x, y});

        if (x > x_max) {
            x_max = x;
        }
        if (y > y_max) {
            y_max = y;
        }
        if (x < x_min) {
            x_min = x;
        }
        if (y < y_min) {
            y_min = y;
        }
    }

    *out_x_min = static_cast<int>(std::round(x_min));
    *out_y_min = static_cast<int>(std::round(y_min));
    *out_width = static_cast<int>(std::round(x_max - x_min));
    *out_height = static_cast<int>(std::round(y_max - y_min));

    PathCoordinate mask_transl{};
    double scale;
    std::tie(mask_transl, scale) = translation_and_scaling(
        width, height, *out_x_min, *out_y_min, out_width, out_height);

    std::string path = transformed_path_string(coordinates, mask_transl, scale);

    // If an odd number of points, add an additional point at the top of the
    // polygon this will shift the calculated center point of the shape so that
    // the center point of the polygon is at x,y (otherwise the center is
    // mis-located)
    if (points % 2 == 1) {
        path = "M0 " + std::to_string(outer_radius) + " " + path;
    }

    return path;
}

std::string Mask::svg_circle_path(const float cx, const float cy,
                                  const float r) const {
    std::ostringstream ss;
    ss << "M " << cx - r << ", " << cy << "a" << r << "," << r << " 0 1,0 "
       << r * 2 << ",0 a " << r << "," << r << " 0 1,0 -" << r * 2 << ",0";
    return ss.str();
}

std::string Mask::svg_ellipse_path(const float cx, const float cy,
                                   const float rx, const float ry) const {
    std::ostringstream ss;
    ss << "M " << cx - rx << ", " << cy << "a" << rx << "," << ry << " 0 1,0 "
       << rx * 2 << ",0a" << rx << "," << ry << " 0 1,0 -" << rx * 2 << ",0";
    return ss.str();
}

std::vector<Mask::PathCoordinate>
Mask::heart_path(const float cx, const float cy, int *out_x_min, int *out_y_min,
                 int *out_width, int *out_height) const {
    std::vector<PathCoordinate> coordinates;

    float x_max = std::numeric_limits<float>::min();
    float y_max = std::numeric_limits<float>::min();
    float x_min = std::numeric_limits<float>::max();
    float y_min = std::numeric_limits<float>::max();

    for (size_t i = 0; i <= 314; ++i) {
        double t = -M_PI + (i * 0.02);
        double x_pt = 16.0 * (std::pow(std::sin(t), 3.0));
        double y_pt = 13.0 * std::cos(t) - 5 * std::cos(2 * t) -
                      2 * std::cos(3 * t) - std::cos(4 * t);

        auto x = static_cast<float>(cx + x_pt * cx);
        auto y = static_cast<float>(cy - y_pt * cy);

        coordinates.push_back({x, y});

        if (x > x_max) {
            x_max = x;
        }
        if (y > y_max) {
            y_max = y;
        }
        if (x < x_min) {
            x_min = x;
        }
        if (y < y_min) {
            y_min = y;
        }
    }

    *out_x_min = static_cast<int>(std::round(x_min));
    *out_y_min = static_cast<int>(std::round(y_min));
    *out_width = static_cast<int>(std::round(x_max - x_min));
    *out_height = static_cast<int>(std::round(y_max - y_min));

    return coordinates;
}

std::pair<Mask::PathCoordinate, double>
Mask::translation_and_scaling(const int image_width, const int image_height,
                              const int mask_x, const int mask_y,
                              int *mask_width, int *mask_height) const {
    // How much bigger is the image relative to the path in each dimension?
    auto scale_based_on_width =
        static_cast<double>(image_width) / static_cast<double>(*mask_width);
    auto scale_based_on_height =
        static_cast<double>(image_height) / static_cast<double>(*mask_height);

    // Of the scaling factors determined in each dimension,
    // use the smaller one; otherwise portions of the path
    // is outside the viewport.
    double scale = std::min(scale_based_on_width, scale_based_on_height);

    // Calculate the bounding box parameters
    // after the path has been scaled relative to the origin
    // but before any subsequent translations have been applied
    double scaled_mask_x = mask_x * scale;
    double scaled_mask_y = mask_y * scale;
    double scaled_mask_width = *mask_width * scale;
    double scaled_mask_height = *mask_height * scale;

    // Calculate the centre points of the scaled but untranslated path
    // as well as of the image
    double scaled_mask_centre_x = scaled_mask_x + (scaled_mask_width / 2.0);
    double scaled_mask_centre_y = scaled_mask_y + (scaled_mask_height / 2.0);
    double image_centre_x = image_width / 2.0;
    double image_centre_y = image_height / 2.0;

    // Calculate translation required to centre the mask
    // on the image
    PathCoordinate mask_transl = {
        static_cast<float>(image_centre_x - scaled_mask_centre_x),
        static_cast<float>(image_centre_y - scaled_mask_centre_y)};

    *mask_width = static_cast<int>(std::round(scaled_mask_width));
    *mask_height = static_cast<int>(std::round(scaled_mask_height));

    return std::make_pair(mask_transl, scale);
}

std::string
Mask::transformed_path_string(const std::vector<PathCoordinate> &coordinates,
                              const PathCoordinate &transl,
                              const double scale) const {
    std::ostringstream ss;
    ss << std::fixed << std::showpoint << std::setprecision(1);

    for (size_t i = 0; i != coordinates.size(); ++i) {
        PathCoordinate coordinate = coordinates[i];

        auto prepend = i == 0 ? "M" : " L";
        ss << prepend << coordinate.x * scale + transl.x << " "
           << coordinate.y * scale + transl.y;
    }

    ss << " Z";

    return ss.str();
}

VImage Mask::process(const VImage &image) const {
    auto mask_type = query_->get<MaskType>("mask", MaskType::None);

    // Should we process the image?
    // Skip for for multi-page images
    if (mask_type == MaskType::None || query_->get<int>("n", 1) > 1) {
        return image;
    }

    int image_width = image.width();
    int image_height = image.height();

    auto preserve_aspect_ratio =
        mask_type == MaskType::Ellipse ? "none" : "xMidYMid meet";

    int x_min, y_min, mask_width, mask_height;
    auto path = svg_path_by_type(image_width, image_height, mask_type, &x_min,
                                 &y_min, &mask_width, &mask_height);

    auto mask_background =
        query_->get<parsers::Color>("mbg", parsers::Color::DEFAULT);
    bool bg_has_alpha = mask_background.has_alpha_channel();

    // Need to copy, since we need to re-assign a few times
    auto output_image = image;

    // Cutout first if the image or mask background has an alpha channel
    if (output_image.has_alpha() || bg_has_alpha) {
        std::ostringstream svg;
        svg << R"(<svg xmlns="http://www.w3.org/2000/svg" version="1.1")"
            << " width=\"" << image_width << "\" height=\"" << image_height
            << "\""
            << " preserveAspectRatio=\"" << preserve_aspect_ratio << "\">\n"
            << "<path d=\"" << path << "\"/>\n"
            << "</svg>";

        auto svg_mask = svg.str();

        auto mask = VImage::new_from_buffer(
            svg_mask, "",
            VImage::option()->set("access", VIPS_ACCESS_SEQUENTIAL));

        // Ensure image to composite is premultiplied sRGB
        mask = mask.premultiply();

        // Cutout via dest-in
        output_image = output_image.composite2(
            mask, VIPS_BLEND_MODE_DEST_IN,
            VImage::option()->set("premultiplied", true));

        // The image now has an alpha channel
        query_->update("has_alpha", true);
    }

    // If the mask background is not completely transparent; overlay the frame
    if (!mask_background.is_transparent()) {
        std::ostringstream svg;
        svg << R"(<svg xmlns="http://www.w3.org/2000/svg" version="1.1")"
            << " width=\"" << image_width << "\" height=\"" << image_height
            << "\""
            << " preserveAspectRatio=\"" << preserve_aspect_ratio << "\">\n"
            << "<path d=\"" << path << " M0 0 h" << image_width << " v"
            << image_height << " h-" << image_width
            << R"( Z" fill-rule="evenodd" )"
            << "fill=\"" << mask_background.to_string() << "\"/>\n"
            << "</svg>";

        auto svg_frame = svg.str();

        auto frame = VImage::new_from_buffer(
            svg_frame, "",
            VImage::option()->set("access", VIPS_ACCESS_SEQUENTIAL));

        // Ensure image to composite is premultiplied sRGB
        frame = frame.premultiply();

        // Alpha composite src over dst
        output_image = output_image.composite2(
            frame, VIPS_BLEND_MODE_OVER,
            VImage::option()->set("premultiplied", true));
    }

    // Crop the image to the mask dimensions;
    // if the mask type is not a ellipse and trimming is needed
    if (mask_type != MaskType::Ellipse &&
        (mask_width < image_width || mask_height < image_height) &&
        query_->get<bool>("mtrim", false)) {
        auto left =
            static_cast<int>(std::round((image_width - mask_width) / 2.0));
        auto top =
            static_cast<int>(std::round((image_height - mask_height) / 2.0));

        return output_image.extract_area(left, top, mask_width, mask_height);
    }

    return output_image;
}

}  // namespace processors
}  // namespace api
}  // namespace weserv
