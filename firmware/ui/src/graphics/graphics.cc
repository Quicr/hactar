#include "graphics.hh"

bool Graphics::Rasterize(Shape& shape,
                         uint8_t* buff,
                         size_t size,
                         const uint16_t window_x1,
                         const uint16_t window_y1,
                         const uint16_t window_x2,
                         const uint16_t window_y2)
{
}

// bool Graphics::RasterizeRectangle(Shape& shape,
//                                   uint8_t* buff,
//                                   size_t size,
//                                   const uint16_t window_x1,
//                                   const uint16_t window_y1,
//                                   const uint16_t window_x2,
//                                   const uint16_t window_y2)
// {
//     if (shape.pixel.x < window_x1 || shape.pixel.x > window_x2)
//
//         // Get the y bounds
//         YBound bound = GetYBounds(y1, y2, memory.y1, memory.y2);
//
//     uint8_t colour_high = (uint8_t)memory.colour << 4;
//     uint8_t colour_low = (uint8_t)memory.colour & 0x0F;
//
//     for (uint16_t i = bound.y1; i < bound.y2; ++i)
//     {
//         for (uint16_t j = memory.x1; j < memory.x2; ++j)
//         {
//             FillMatrixAtIdx(matrix, i, j, colour_high, colour_low);
//         }
//     }
//
//     return true;
// }
//
// void Screen::DrawRectangleProcedure(const int16_t x1,
//                                     const int16_t x2,
//                                     const int16_t y1,
//                                     const int16_t y2,
//                                     const uint16_t thickness,
//                                     const Colour colour)
// {
//     // TODO flash fill of matrix
// }
//
// bool Screen::DrawRectangleProcedure(DrawMemory& memory,
//                                     uint8_t matrix[HEIGHT][Half_Width_Pixel_Size],
//                                     const int16_t y1,
//                                     const int16_t y2)
// {
//     // See if rectangle is in the current scan line
//     if (y1 >= memory.y2 || y2 <= memory.y1)
//     {
//         return false;
//     }
//
//     const uint16_t thickness = PullMemoryParameter<uint16_t>(memory);
//
//     // Get the y bounds
//     YBound bound = GetYBounds(y1, y2, memory.y1, memory.y2);
//
//     uint8_t colour_high = (uint8_t)memory.colour << 4;
//     uint8_t colour_low = (uint8_t)memory.colour & 0x0F;
//
//     // Check for the "top rectangle"
//     if (y1 <= memory.y1)
//     {
//         const uint16_t y1_thick = bound.y1 + thickness < bound.y2 ? bound.y1 + thickness :
//         bound.y2;
//         // In the bounds of the top rectangle
//         for (uint16_t i = bound.y1; i < y1_thick; ++i)
//         {
//             for (uint16_t j = memory.x1; j < memory.x2; ++j)
//             {
//                 FillMatrixAtIdx(matrix, i, j, colour_high, colour_low);
//             }
//         }
//     }
//
//     // Check for the "bottom rectangle"
//     if (y2 >= memory.y2)
//     {
//         const uint16_t y2_thick = bound.y2 - thickness > bound.y1 ? bound.y2 - thickness :
//         bound.y1;
//         // In the bounds of the top rectangle
//         for (uint16_t i = y2_thick; i < bound.y2; ++i)
//         {
//             for (uint16_t j = memory.x1; j < memory.x2; ++j)
//             {
//                 FillMatrixAtIdx(matrix, i, j, colour_high, colour_low);
//             }
//         }
//     }
//
//     const uint16_t x1_thick = memory.x1 + thickness;
//     const uint16_t x2_thick = memory.x2 - thickness;
//     // Do the side rectangles
//     for (uint16_t i = bound.y1; i < bound.y2; ++i)
//     {
//         uint16_t left_v = memory.x1;
//         uint16_t right_v = x2_thick;
//         while (left_v < x1_thick)
//         {
//             FillMatrixAtIdx(matrix, i, left_v, colour_high, colour_low);
//             FillMatrixAtIdx(matrix, i, right_v, colour_high, colour_low);
//             ++right_v;
//             ++left_v;
//         }
//     }
//
//     return true;
// }
//
// bool Screen::DrawCharacterProcedure(DrawMemory& memory,
//                                     uint8_t matrix[HEIGHT][Half_Width_Pixel_Size],
//                                     const int16_t y1,
//                                     const int16_t y2)
// {
//     if (y1 >= memory.y2 || y2 <= memory.y1)
//     {
//         return false;
//     }
//
//     const uint8_t bg = PullMemoryParameter<uint8_t>(memory);
//     uint8_t* ch_ptr = (uint8_t*)PullMemoryParameter<uint32_t>(memory);
//
//     uint16_t w_off = 0;
//
//     YBound bounds = GetYBounds(y1, y2, memory.y1, memory.y2);
//
//     uint8_t fg_high = (uint8_t)memory.colour << 4;
//     uint8_t fg_low = (uint8_t)memory.colour & 0x0F;
//     uint8_t bg_high = bg << 4;
//     uint8_t bg_low = bg & 0x0F;
//
//     for (uint16_t i = bounds.y1; i < bounds.y2; ++i)
//     {
//         w_off = 0;
//         for (uint16_t j = memory.x1; j < memory.x2; ++j)
//         {
//             if ((*ch_ptr << w_off) & 0x80)
//             {
//                 FillMatrixAtIdx(matrix, i, j, fg_high, fg_low);
//             }
//             else
//             {
//                 FillMatrixAtIdx(matrix, i, j, bg_high, bg_low);
//             }
//
//             ++w_off;
//             if (w_off >= 8)
//             {
//                 w_off = 0;
//
//                 // At the end of the byte's bits, if a font > 8 bits
//                 // then we need to slide to the next byte. which is in the same
//                 // column
//                 ++ch_ptr;
//             }
//         }
//
//         // Slide the pointer to the next row byte
//         ++ch_ptr;
//     }
//
//     ch_ptr = nullptr;
//
//     return true;
// }
//
// bool Screen::DrawStringProcedure(DrawMemory& memory,
//                                  uint8_t matrix[HEIGHT][Half_Width_Pixel_Size],
//                                  const int16_t y1,
//                                  const int16_t y2)
// {
//     // TODO put into function
//     if (y1 >= memory.y2 || y2 <= memory.y1)
//     {
//         return false;
//     }
//
//     const uint8_t bg = PullMemoryParameter<uint8_t>(memory);
//     const uint8_t* str = (uint8_t*)PullMemoryParameter<uint32_t>(memory);
//     const uint8_t font_width = PullMemoryParameter<uint8_t>(memory);
//     const uint8_t font_height = PullMemoryParameter<uint8_t>(memory);
//     uint8_t* font_data = (uint8_t*)PullMemoryParameter<uint32_t>(memory);
//
//     const uint16_t bytes_per_char = (font_width / 8) + 1;
//
//     uint16_t ch_idx = 0;
//     uint8_t* ch_ptr = nullptr;
//
//     // How many iterations we've been at the same character
//     // If it exceeds the font width that means we are on the next
//     // character in the string
//     uint16_t x_char_iter = 0;
//     uint16_t w_off = 0;
//
//     // Get the current number of scan line already passed
//     uint16_t y_char_iter = 0;
//     if (y1 > memory.y1)
//     {
//         // We have done some line
//         y_char_iter = (y1 - memory.y1);
//     }
//
//     YBound bounds = GetYBounds(y1, y2, memory.y1, memory.y2);
//
//     uint8_t fg_high = (uint8_t)memory.colour << 4;
//     uint8_t fg_low = (uint8_t)memory.colour & 0x0F;
//     uint8_t bg_high = bg << 4;
//     uint8_t bg_low = bg & 0x0F;
//
//     for (uint16_t i = bounds.y1; i < bounds.y2; ++i)
//     {
//         x_char_iter = 0;
//         w_off = 0;
//         ch_idx = 0;
//         ch_ptr = GetCharAddr(font_data, str[ch_idx], font_width, font_height)
//                + (y_char_iter * bytes_per_char);
//
//         for (uint16_t j = memory.x1; j < memory.x2; ++j)
//         {
//             if ((*ch_ptr << w_off) & 0x80)
//             {
//                 FillMatrixAtIdx(matrix, i, j, fg_high, fg_low);
//             }
//             else
//             {
//                 FillMatrixAtIdx(matrix, i, j, bg_high, bg_low);
//             }
//
//             ++w_off;
//             ++x_char_iter;
//
//             // Check if we are on the next character
//             if (x_char_iter >= font_width)
//             {
//                 x_char_iter = 0;
//
//                 // Get the character
//                 ++ch_idx;
//                 ch_ptr = GetCharAddr(font_data, str[ch_idx], font_width, font_height)
//                        + (y_char_iter * bytes_per_char);
//
//                 // Next character and reset w_off
//                 w_off = 0;
//             }
//
//             if (w_off >= 8)
//             {
//                 w_off = 0;
//
//                 // At the end of the byte's bits, if a font > 8 bits
//                 // then we need to slide to the next byte. which is in the same
//                 // column
//                 ++ch_ptr;
//             }
//         }
//
//         // Slide the pointer is the next row
//         ++y_char_iter;
//     }
//
//     ch_ptr = nullptr;
//     font_data = nullptr;
//
//     return true;
// }
//
// void Screen::HandleBounds(uint16_t& x1, uint16_t& x2, uint16_t& y1, uint16_t& y2)
// {
//     if (x1 >= view_width || y1 >= view_height)
//     {
//         return;
//     }
//
//     if (x2 >= view_width)
//     {
//         x2 = view_width;
//     }
//
//     if (x1 > x2)
//     {
//         const uint16_t tmp = x1;
//         x1 = x2;
//         x2 = tmp;
//     }
//
//     if (y1 > y2)
//     {
//         const uint16_t tmp = y1;
//         y1 = y2;
//         y2 = tmp;
//     }
// }
//
// inline uint8_t* Screen::GetCharAddr(uint8_t* font_data,
//                                     const uint8_t ch,
//                                     const uint16_t font_width,
//                                     const uint16_t font_height)
// {
//     return font_data + ((ch - 32) * font_height * (font_width / 8 + 1));
// }
//
// inline void
// Screen::PushMemoryParameter(DrawMemory& memory, const uint32_t val, const int16_t num_bytes)
// {
//     const int16_t bytes = num_bytes < 4 ? num_bytes : 4;
//
//     uint8_t& idx = memory.write_idx;
//     for (int16_t i = 0; i < bytes; ++i)
//     {
//         // We are relying on truncation to save a cycle
//         memory.parameters[idx] = (val >> (8 * i));
//         ++idx;
//     }
// }
//
// // TODO colour HIGH and colour LOW to save calculating it everytime.
// // Each byte stores two pixels of colour.
// // The first half is the "even" pixel and the second half is the "odd" pixel
// inline void Screen::FillMatrixAtIdx(uint8_t matrix[HEIGHT][Half_Width_Pixel_Size],
//                                     const uint16_t i,
//                                     const uint16_t j,
//                                     const uint8_t colour_high,
//                                     const uint8_t colour_low)
// {
//     const uint16_t idx = j / 2;
//     if (j & 0x0001)
//     {
//         matrix[i][idx] = (matrix[i][idx] & 0xF0) | colour_low;
//     }
//     else
//     {
//         matrix[i][idx] = (matrix[i][idx] & 0x0F) | colour_high;
//     }
// }
//
// inline Screen::YBound Screen::GetYBounds(const uint16_t y1,
//                                          const uint16_t y2,
//                                          const uint16_t mem_y1,
//                                          const uint16_t mem_y2)
// {
//     const uint16_t y_start = y1 > mem_y1 ? y1 : mem_y1;
//     const uint16_t y_end = y2 < mem_y2 ? y2 : mem_y2;
//
//     // const uint16_t y_start = y1 > mem_y1 ? 0 : mem_y1 - y1;
//     // const uint16_t y_end = y2 < mem_y2 ? y2 - y1 : mem_y2 - y1;
//
//     return {y_start, y_end};
// }
