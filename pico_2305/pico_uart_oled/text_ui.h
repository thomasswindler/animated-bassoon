#pragma once

void textui_init(void);
void textui_clear(void);

void textui_putc(char ch);

// Convenience for your button message:
void textui_write_row0(const char *msg);   // clears row 0 and writes msg

void textui_render(void);
