-- theme.lua

ui_font_base_size = font_size
ui_line_spacing = font_size

themes = {
--           main bg    bg        border    button    text      border thickness  title bar padding   button roundness  slider inner padding  slider knob size
  default = { "242430", "555a75", "000000", "9aaaff", "ffffff", 0.0,              8,                  0.1,              8,                    8 },
  gray =    { "373737", "4b4b4b", "232323", "5568a0", "f0f0f0", 2.0,              8,                  0.3,              8,                    8 },
}

function load_theme(name)
  local theme = themes[name]
  if theme ~= nil then
    background_color = theme[1]
    ui_background_color = theme[2]
    ui_border_color = theme[3]
    ui_button_color = theme[4]
    ui_text_color = theme[5]
    ui_border_thickness = theme[6]
    ui_title_bar_padding = theme[7]
    ui_button_roundness = theme[8]
    ui_slider_inner_padding = theme[9]
    ui_slider_knob_size = theme[10]
  end
end

load_theme("default")
