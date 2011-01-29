void Window::create(unsigned x, unsigned y, unsigned width, unsigned height, const string &text) {
  widget->window = CreateWindowEx(
    0, L"phoenix_window", utf16_t(text),
    WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX,
    window->x = x, window->y = y, window->width = width, window->height = height,
    0, 0, GetModuleHandle(0), 0
  );
  window->menu = CreateMenu();
  window->status = CreateWindowEx(
    0, STATUSCLASSNAME, L"",
    WS_CHILD,
    0, 0, 0, 0,
    widget->window, 0, GetModuleHandle(0), 0
  );
  //StatusBar will be capable of receiving tab focus if it is not disabled
  SetWindowLongPtr(window->status, GWL_STYLE, GetWindowLong(window->status, GWL_STYLE) | WS_DISABLED);
  resize(width, height);
  SetWindowLongPtr(widget->window, GWLP_USERDATA, (LONG_PTR)this);
}

void Window::setDefaultFont(Font &font) {
  window->defaultFont = font.font->font;
}

void Window::setFont(Font &font) {
  SendMessage(window->status, WM_SETFONT, (WPARAM)font.font->font, 0);
}

Geometry Window::geometry() {
  RECT position, size;
  GetWindowRect(widget->window, &position);
  GetClientRect(widget->window, &size);
  if(GetWindowLongPtr(window->status, GWL_STYLE) & WS_VISIBLE) {
    RECT status;
    GetClientRect(window->status, &status);
    size.bottom -= status.bottom - status.top;
  }
  return Geometry(position.left, position.top, size.right, size.bottom);
}

void Window::setGeometry(unsigned x, unsigned y, unsigned width, unsigned height) {
  if(window->isFullscreen == false) {
    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
  }

  bool isVisible = visible();
  if(isVisible) setVisible(false);
  SetWindowPos(widget->window, NULL, x, y, width, height, SWP_NOZORDER | SWP_FRAMECHANGED);
  resize(width, height);
  if(isVisible) setVisible(true);
}

void Window::setBackgroundColor(uint8_t red, uint8_t green, uint8_t blue) {
  if(window->brush) DeleteObject(window->brush);
  window->brushColor = RGB(red, green, blue);
  window->brush = CreateSolidBrush(window->brushColor);
}

void Window::setTitle(const string &text) {
  SetWindowText(widget->window, utf16_t(text));
}

void Window::setStatusText(const string &text) {
  SendMessage(window->status, SB_SETTEXT, 0, (LPARAM)(wchar_t*)utf16_t(text));
}

void Window::setMenuVisible(bool visible) {
  SetMenu(widget->window, visible ? window->menu : 0);
  resize(window->width, window->height);
}

void Window::setStatusVisible(bool visible) {
  ShowWindow(window->status, visible ? SW_SHOWNORMAL : SW_HIDE);
  resize(window->width, window->height);
}

bool Window::fullscreen() {
  return window->isFullscreen;
}

void Window::setFullscreen(bool fullscreen) {
  window->isFullscreen = fullscreen;
  if(fullscreen == false) {
    SetWindowLong(widget->window, GWL_STYLE, WS_VISIBLE | WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX);
    setGeometry(window->x, window->y, window->width, window->height);
  } else {
    SetWindowLong(widget->window, GWL_STYLE, WS_VISIBLE | WS_POPUP);
    setGeometry(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
  }
}

Window::Window() {
  window = new Window::Data;
  window->defaultFont = 0;
  window->brush = 0;
  window->isFullscreen = false;
  window->x = 0;
  window->y = 0;
  window->width = 0;
  window->height = 0;
}

void Window::resize(unsigned width, unsigned height) {
  SetWindowPos(widget->window, NULL, 0, 0, width, height, SWP_NOZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);
  RECT rc;
  GetClientRect(widget->window, &rc);
  width += width - (rc.right - rc.left);
  height += height - (rc.bottom - rc.top);

  if(GetWindowLongPtr(window->status, GWL_STYLE) & WS_VISIBLE) {
    GetClientRect(window->status, &rc);
    height += rc.bottom - rc.top;
  }

  SetWindowPos(widget->window, NULL, 0, 0, width, height, SWP_NOZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);
  SetWindowPos(window->status, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_FRAMECHANGED);
}