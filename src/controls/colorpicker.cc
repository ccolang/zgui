//@zgui_packer:ignore
#include "../zgui.hh"

using namespace zgui::globals;


//@zgui_packer:resume
// ========================================================================
namespace {
    struct hsv_t { float h, s, v, a; };

    hsv_t rgb_to_hsv(zgui::color c) {
        float r = c.r / 255.0f;
        float g = c.g / 255.0f;
        float b = c.b / 255.0f;
        float a = c.a / 255.0f;

        float max_val = std::max({ r, g, b });
        float min_val = std::min({ r, g, b });
        float delta = max_val - min_val;

        float h = 0, s = 0, v = max_val;

        if (delta > 0) {
            if (max_val == r) h = (g - b) / delta + (g < b ? 6 : 0);
            else if (max_val == g) h = (b - r) / delta + 2;
            else h = (r - g) / delta + 4;
            h /= 6;
            s = delta / max_val;
        }
        return { h, s, v, a };
    }

    zgui::color hsv_to_rgb(float h, float s, float v, float a) {
        float r, g, b;
        int i = static_cast<int>(h * 6);
        float f = h * 6 - i;
        float p = v * (1 - s);
        float q = v * (1 - f * s);
        float t = v * (1 - (1 - f) * s);

        switch (i % 6) {
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
        default: r = v, g = t, b = p; break;
        }

        return {
            static_cast<int>(r * 255),
            static_cast<int>(g * 255),
            static_cast<int>(b * 255),
            static_cast<int>(a * 255)
        };
    }
}

void zgui::colorpicker(const char* id, color& value) {
    std::vector<std::string> id_split = utils::hash::split_str(id, '#');

    const int control_width = 30;
    const int control_height = 14;

    const unsigned long font = utils::misc::pop_font();

    const vec2 cursor_pos = utils::misc::pop_cursor_pos();
    vec2 draw_pos{ window_ctx.position.x + cursor_pos.x + 14, window_ctx.position.y + cursor_pos.y };

    const bool inlined = id_split[0].empty();

    if (!inlined) {
        int text_width, text_height;
        functions.get_text_size(font, id_split[0].c_str(), text_width, text_height);

        window_ctx.render.emplace_back(zgui_control_render_t{ {draw_pos.x, draw_pos.y - 4}, zgui_render_type::zgui_text, global_colors.color_text, id_split[0], vec2{0, 0}, font });

        draw_pos.y += text_height;
    }

    window_ctx.render.emplace_back(zgui_control_render_t{ {draw_pos.x, draw_pos.y}, zgui_render_type::zgui_rect, global_colors.control_outline, "", {static_cast<float>(control_width), static_cast<float>(control_height)} });
    window_ctx.render.emplace_back(zgui_control_render_t{ {draw_pos.x + 1, draw_pos.y + 1}, zgui_render_type::zgui_filled_rect, value, "", {static_cast<float>(control_width) - 2, static_cast<float>(control_height) - 2} });

    if (const bool hovered = utils::input::mouse_in_region(draw_pos.x, draw_pos.y, control_width, control_height); hovered && utils::input::key_pressed(VK_LBUTTON) && window_ctx.blocking == 0) {
        window_ctx.blocking = utils::hash::hash(id);
    }
    else if (window_ctx.blocking == utils::hash::hash(id)) {
        const int popup_width = 162;
        const int popup_height = 160;
        const int popup_padding = 6;

        vec2 popup_pos = { draw_pos.x, draw_pos.y + control_height + 4 };

        if (!utils::input::mouse_in_region(popup_pos.x, popup_pos.y, popup_width, popup_height + 30) && utils::input::key_pressed(VK_LBUTTON) && !utils::input::mouse_in_region(draw_pos.x, draw_pos.y, control_width, control_height)) {
            window_ctx.blocking = 0;
        }

        window_ctx.render.emplace_back(zgui_control_render_t{ {popup_pos.x, popup_pos.y}, zgui_render_type::zgui_rect, global_colors.control_outline, "", {static_cast<float>(popup_width), static_cast<float>(popup_height)} });

        hsv_t hsv = rgb_to_hsv(value);

        const int sv_box_size = 150;
        const int sv_box_x = popup_pos.x + popup_padding;
        const int sv_box_y = popup_pos.y + popup_padding;

        float cursor_x = sv_box_x + (hsv.s * sv_box_size);
        float cursor_y = sv_box_y + ((1.0f - hsv.v) * sv_box_size);
        window_ctx.render.emplace_back(zgui_control_render_t{ {cursor_x - 2, cursor_y - 2}, zgui_render_type::zgui_rect, {255, 255, 255, 255}, "", {4, 4} });

        if (utils::input::key_down(VK_LBUTTON) && utils::input::mouse_in_region(sv_box_x, sv_box_y, sv_box_size, sv_box_size)) {
            hsv.s = std::clamp((mouse_pos.x - sv_box_x) / static_cast<float>(sv_box_size), 0.0f, 1.0f);
            hsv.v = std::clamp(1.0f - ((mouse_pos.y - sv_box_y) / static_cast<float>(sv_box_size)), 0.0f, 1.0f);
            color new_col = hsv_to_rgb(hsv.h, hsv.s, hsv.v, hsv.a);
            value.r = new_col.r; value.g = new_col.g; value.b = new_col.b;
        }

        for (int i = 0; i < sv_box_size; i += 2) {
            for (int j = 0; j < sv_box_size; j += 2) {
                float s = static_cast<float>(i) / sv_box_size;
                float v = 1.0f - (static_cast<float>(j) / sv_box_size);
                color c = hsv_to_rgb(hsv.h, s, v, 1.0f);
                window_ctx.render.emplace_back(zgui_control_render_t{ {static_cast<float>(sv_box_x + i), static_cast<float>(sv_box_y + j)}, zgui_render_type::zgui_filled_rect, c, "", {static_cast<float>(2), static_cast<float>(2)} });
            }
        }
        window_ctx.render.emplace_back(zgui_control_render_t{ {static_cast<float>(sv_box_x - 1), static_cast<float>(sv_box_y - 1)}, zgui_render_type::zgui_rect, global_colors.control_outline, "", {static_cast<float>(sv_box_size + 2), static_cast<float>(sv_box_size + 2)} });

        const int bar_height = 10;
        const int bar_y = sv_box_y + sv_box_size + popup_padding;
        window_ctx.render.emplace_back(zgui_control_render_t{ {static_cast<float>(sv_box_x + (hsv.h * sv_box_size) - 1), static_cast<float>(bar_y)}, zgui_render_type::zgui_rect, {255, 255, 255, 255}, "", {3, static_cast<float>(bar_height)} });

        if (utils::input::key_down(VK_LBUTTON) && utils::input::mouse_in_region(sv_box_x, bar_y, sv_box_size, bar_height)) {
            hsv.h = std::clamp((mouse_pos.x - sv_box_x) / static_cast<float>(sv_box_size), 0.0f, 1.0f);
            color new_col = hsv_to_rgb(hsv.h, hsv.s, hsv.v, hsv.a);
            value.r = new_col.r; value.g = new_col.g; value.b = new_col.b;
        }

        for (int i = 0; i < sv_box_size; ++i) {
            float h = static_cast<float>(i) / sv_box_size;
            color c = hsv_to_rgb(h, 1.0f, 1.0f, 1.0f);
            window_ctx.render.emplace_back(zgui_control_render_t{ {static_cast<float>(sv_box_x + i), static_cast<float>(bar_y)}, zgui_render_type::zgui_filled_rect, c, "", {1, static_cast<float>(bar_height)} });
        }
        window_ctx.render.emplace_back(zgui_control_render_t{ {static_cast<float>(sv_box_x - 1), static_cast<float>(bar_y - 1)}, zgui_render_type::zgui_rect, global_colors.control_outline, "", {static_cast<float>(sv_box_size + 2), static_cast<float>(bar_height + 2)} });


        const int alpha_bar_y = bar_y + bar_height + popup_padding;

        if (utils::input::key_down(VK_LBUTTON) && utils::input::mouse_in_region(sv_box_x, alpha_bar_y, sv_box_size, bar_height)) {
            float a = std::clamp((mouse_pos.x - sv_box_x) / static_cast<float>(sv_box_size), 0.0f, 1.0f);
            value.a = static_cast<int>(a * 255);
        }

        float alpha_fraction = value.a / 255.0f;
        window_ctx.render.emplace_back(zgui_control_render_t{ {static_cast<float>(sv_box_x + (alpha_fraction * sv_box_size) - 1), static_cast<float>(alpha_bar_y)}, zgui_render_type::zgui_rect, {255, 255, 255, 255}, "", {3, static_cast<float>(bar_height)} });


        window_ctx.render.emplace_back(zgui_control_render_t{ {static_cast<float>(sv_box_x), static_cast<float>(alpha_bar_y)}, zgui_render_type::zgui_filled_rect, {100, 100, 100, 255}, "", {static_cast<float>(sv_box_size), static_cast<float>(bar_height)} });

        color start_col = value; start_col.a = 0;
        color end_col = value; end_col.a = 255;

        for (int i = 0; i < sv_box_size; i += 2) {
            float a = static_cast<float>(i) / sv_box_size;
            color c = value;
            c.a = static_cast<int>(a * 255);
            window_ctx.render.emplace_back(zgui_control_render_t{ {static_cast<float>(sv_box_x + i), static_cast<float>(alpha_bar_y)}, zgui_render_type::zgui_filled_rect, c, "", {2, static_cast<float>(bar_height)} });
        }

        window_ctx.render.emplace_back(zgui_control_render_t{ {static_cast<float>(sv_box_x - 1), static_cast<float>(alpha_bar_y - 1)}, zgui_render_type::zgui_rect, global_colors.control_outline, "", {static_cast<float>(sv_box_size + 2), static_cast<float>(bar_height + 2)} });
    }

    utils::misc::push_cursor_pos(vec2{ cursor_pos.x + control_width + global_config.item_spacing, cursor_pos.y });
    utils::misc::push_cursor_pos(vec2{ cursor_pos.x, cursor_pos.y + control_height / 2 + global_config.item_spacing + (inlined ? 0 : 12) });

    utils::misc::push_font(font);
}
// ========================================================================