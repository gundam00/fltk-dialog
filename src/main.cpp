/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016-2017, djcj <djcj@gmx.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "misc/args.hxx"  /* include first */

#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Double_Window.H>
#ifdef WITH_DEFAULT_ICON
#  include <FL/Fl_Pixmap.H>
#  include <FL/Fl_RGB_Image.H>
#endif

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <string.h>

#include "fltk-dialog.hpp"
#include "misc/split.hpp"

#ifdef WITH_DEFAULT_ICON
#  include "icon.xpm"
#endif

typedef args::Flag ARG_T;
typedef args::ValueFlag<int> ARGI_T;
typedef args::ValueFlag<long> ARGL_T;
typedef args::ValueFlag<float> ARGF_T;
typedef args::ValueFlag<double> ARGD_T;
typedef args::ValueFlag<std::string> ARGS_T;

#define GETCSTR(a,b)  if (b) { a = args::get(b).c_str(); }
#define GETVAL(a,b)   if (b) { a = args::get(b); }

enum dialogTypes {
  DIALOG_ABOUT,
  DIALOG_CALENDAR,
  DIALOG_CHECKLIST,
  DIALOG_COLOR,
  DIALOG_DATE,
  DIALOG_DIR_CHOOSER,
  DIALOG_DND,
  DIALOG_DROPDOWN,
  DIALOG_FILE_CHOOSER,
  DIALOG_FONT,
  DIALOG_HTML,
  DIALOG_INPUT,
  DIALOG_MESSAGE,
  DIALOG_NOTIFY,
  DIALOG_PASSWORD,
  DIALOG_PROGRESS,
  DIALOG_RADIOLIST,
  DIALOG_QUESTION,
  DIALOG_SCALE,
  DIALOG_TEXTINFO,
  DIALOG_WARNING
};

const char *title = NULL;
const char *msg = NULL;
int ret = 0;

char separator = '|';
std::string separator_s = "|";

bool arabic = false;

/* get dimensions of the main screen work area */
int max_w = Fl::w();
int max_h = Fl::h();

int override_x = -1;
int override_y = -1;
int override_w = -1;
int override_h = -1;
bool resizable = true;
bool position_center = false;
bool window_taskbar = true;
bool window_decoration = true;

double scale_min = 0;
double scale_max = 100;
double scale_step = 1;
double scale_init;

/* don't use fltk's '@' symbols */
#define USE_SYMBOLS 0

/* global FLTK callback for drawing all label text */
static void draw_cb(const Fl_Label *o, int x, int y, int w, int h, Fl_Align a)
{
  fl_font(o->font, o->size);
  fl_color((Fl_Color)o->color);
  fl_draw(o->value, x, y, w, h, a, o->image, USE_SYMBOLS);
}

/* global FLTK callback for measuring all labels */
static void measure_cb(const Fl_Label *o, int &w, int &h)
{
  fl_font(o->font, o->size);
  fl_measure(o->value, w, h, USE_SYMBOLS);
}

static int esc_handler(int event)
{
  if (Fl::event() == FL_SHORTCUT && Fl::event_key() == FL_Escape)
  {
    return 1; /* ignore Escape key */
  }
  return 0;
}

void set_size(Fl_Double_Window *o, Fl_Widget *w)
{
  if (resizable)
  {
    o->resizable(w);
  }

  if (override_w > 0)
  {
    o->size(override_w, o->h());
  }

  if (override_h > 0)
  {
    o->size(o->w(), override_h);
  }
}

void set_position(Fl_Double_Window *o)
{
  if (position_center)
  {
    override_x = (max_w - o->w()) / 2;
    override_y = (max_h - o->h()) / 2;
  }

  if (override_x >= 0)
  {
    o->position(override_x, o->y());
  }

  if (override_y >= 0)
  {
    o->position(o->x(), override_y);
  }
}

/* place before show() */
void set_taskbar(Fl_Double_Window *o)
{
  if (!window_taskbar)
  {
    o->border(0);
  }
}

/* place after show() */
void set_undecorated(Fl_Double_Window *o)
{
  if (window_decoration)
  {
    o->border(1);
  }
  else
  {
    o->border(0);
  }
}

#define STRINGTOINT(s, a, b)  if (_argtoint(s.c_str(), a, argv[0], b)) { return 1; }
static int _argtoint(const char *arg, int &val, const char *self, std::string cmd)
{
  char *p;
  long l = strtol(arg, &p, 10);

  if (*p)
  {
    std::cerr << self << ": " << cmd << ": input is not an integer number" << std::endl;
    return 1;
  }
  val = (int) l;
  return 0;
}

static int use_only_with(const char *self, std::string a, std::string b)
{
  std::cerr << self << ": " << a << " can only be used with " << b << "\n"
    "See `" << self << " --help' for more information" << std::endl;
  return 1;
}

int main(int argc, char **argv)
{
#ifdef WITH_DEFAULT_ICON
  Fl_Pixmap win_pixmap(icon_xpm);
  Fl_RGB_Image win_icon(&win_pixmap, Fl_Color(0));
  Fl_Window::default_icon(&win_icon);
#endif

#ifdef WITH_L10N
  l10n();
#endif

  /* recommended in Fl_Double_Window.H */
  Fl::visual(FL_DOUBLE|FL_INDEX);

  const char *scheme_default = "gtk+";

  if (argc < 2)
  {
    Fl::set_labeltype(FL_NORMAL_LABEL, draw_cb, measure_cb); /* disable fltk's '@' symbols */
    Fl::scheme(scheme_default);
    Fl::get_system_colors();
    return about();
  }

  args::ArgumentParser ap_main("FLTK dialog - run dialog boxes from shell scripts", "");

  args::Group ap(ap_main, "Generic options:");
  args::HelpFlag help(ap, "help", "Show options", {'h', "help"});
  ARG_T arg_version(ap, "version", "Show FLTK and program version", {'v', "version"});
  ARG_T arg_about(ap, "about", "About FLTK dialog", {"about"});
  ARGS_T arg_text(ap, "TEXT", "Set the dialog text", {"text"});
  ARGS_T arg_title(ap, "TEXT", "Set the dialog title", {"title"});
  ARGS_T arg_ok_label(ap, "TEXT", "Set the OK button text", {"ok-label"});
  ARGS_T arg_cancel_label(ap, "TEXT", "Set the CANCEL button text", {"cancel-label"});
  ARGS_T arg_close_label(ap, "TEXT", "Set the CLOSE button text", {"close-label"});
  ARGS_T arg_separator(ap, "SEPARATOR", "Set common separator character", {"separator"});
#ifdef WITH_WINDOW_ICON
  ARGS_T arg_window_icon(ap, "FILE", "Set the window icon; supported are: bmp gif jpg png svg svgz xbm xpm", {"window-icon"});
#endif
  ARGI_T arg_width(ap, "WIDTH", "Set the window width", {"width"});
  ARGI_T arg_height(ap, "HEIGHT", "Set the window height", {"height"});
  ARGI_T arg_posx(ap, "NUMBER", "Set the X position of a window", {"posx"});
  ARGI_T arg_posy(ap, "NUMBER", "Set the Y position of a window", {"posy"});
  ARGS_T arg_geometry(ap, "WxH+X+Y", "Set the window geometry", {"geometry"});
  ARG_T arg_fixed(ap, "fixed", "Set window unresizable", {"fixed"});
  ARG_T arg_center(ap, "center", "Place window at center of screen", {"center"});
  ARG_T arg_no_escape(ap, "no-escape", "Don't close window when hitting ESC button", {"no-escape"});
  ARGS_T arg_scheme(ap, "NAME", "Set the window scheme to use: default, gtk+, gleam, plastic or simple; default is gtk+", {"scheme"});
  ARG_T arg_no_system_colors(ap, "no-system-colors", "Use FLTK's default gray color scheme", {"no-system-colors"});
  ARG_T arg_undecorated(ap, "undecorated", "Set window undecorated (doesn't work on file/directory selection)", {"undecorated"});
  ARG_T arg_skip_taskbar(ap, "skip-taskbar", "Don't show window in taskbar", {"skip-taskbar"});
  ARG_T arg_message(ap, "message", "Display message dialog", {"message"});
  ARG_T arg_warning(ap, "warning", "Display warning dialog", {"warning"});
  ARG_T arg_question(ap, "question", "Display question dialog", {"question"});
#ifdef WITH_DND
  ARG_T arg_dnd(ap, "dnd", "Display drag-n-drop box", {"dnd"});
#endif
#ifdef WITH_FILE
  ARG_T arg_file(ap, "file", "Display file selection dialog", {"file"});
  ARG_T arg_directory(ap, "directory", "Display directory selection dialog", {"directory"});
#endif
  ARG_T arg_entry(ap, "entry", "Display text entry dialog", {"entry"});
  ARG_T arg_password(ap, "password", "Display password dialog", {"password"});
#ifdef WITH_PROGRESS
  ARG_T arg_progress(ap, "progress", "Display progress indication dialog", {"progress"});
#endif
#ifdef WITH_CALENDAR
  ARG_T arg_calendar(ap, "calendar", "Display calendar dialog; returns date as Y-M-D", {"calendar"});
#endif
#ifdef WITH_DATE
  ARG_T arg_date(ap, "date", "Display date selection dialog; returns date as Y-M-D", {"date"});
#endif
#ifdef WITH_COLOR
  ARG_T arg_color(ap, "color", "Display color selection dialog; returns color as \"RGB [0.000-1.000]|RGB [0-255]|HTML hex|HSV\"", {"color"});
#endif
  ARG_T arg_scale(ap, "scale", "Display scale dialog", {"scale"});
#ifdef WITH_CHECKLIST
  ARGS_T arg_checklist(ap, "OPT1|OPT2[|..]", "Display a check button list", {"checklist"});
#endif
#ifdef WITH_RADIOLIST
  ARGS_T arg_radiolist(ap, "OPT1|OPT2[|..]", "Display a radio button list", {"radiolist"});
#endif
#ifdef WITH_DROPDOWN
  ARGS_T arg_dropdown(ap, "OPT1|OPT2[|..]", "Display a dropdown menu", {"dropdown"});
#endif
#ifdef WITH_HTML
  ARGS_T arg_html(ap, "FILE", "Display HTML viewer", {"html"});
#endif
#ifdef WITH_TEXTINFO
  ARG_T arg_text_info(ap, "text-info", "Display text information dialog", {"text-info"});
#endif
#ifdef WITH_NOTIFY
  ARG_T arg_notification(ap, "notification", "Display a notification pop-up", {"notification"});
#endif
#ifdef WITH_FONT
  ARG_T arg_font(ap, "font", "Display font selection dialog", {"font"});
#endif

  args::Group g_mwq_options(ap_main, "Message/warning/question options:");
  ARG_T arg_no_symbol(g_mwq_options, "no-symbol", "Don't show symbol box", {"no-symbol"});

  args::Group g_question_options(ap_main, "Question options:");
  ARGS_T arg_yes_label(g_question_options, "TEXT", "Sets the label of the Yes button", {"yes-label"});
  ARGS_T arg_no_label(g_question_options, "TEXT", "Sets the label of the No button", {"no-label"});
  ARGS_T arg_alt_label(g_question_options, "TEXT", "Adds a third button and sets its label; exit code is 2", {"alt-label"});

#if defined(WITH_FILE) && defined(WITH_NATIVE_FILE_CHOOSER)
  args::Group g_file_dir_options(ap_main, "File/directory selection options:");
  ARG_T arg_native(g_file_dir_options, "native", "Use the operating system's native file chooser if available, otherwise fall back to FLTK's own version", {"native"});
#  ifdef HAVE_QT
  ARG_T arg_native_gtk(g_file_dir_options, "native-gtk", "Display the Gtk+ native file chooser", {"native-gtk"});
#    ifdef HAVE_QT4
  ARG_T arg_native_qt4(g_file_dir_options, "native-qt4", "Display the Qt4 native file chooser", {"native-qt4"});
#    endif
#    ifdef HAVE_QT5
  ARG_T arg_native_qt5(g_file_dir_options, "native-qt5", "Display the Qt5 native file chooser", {"native-qt5"});
#    endif
  ARG_T arg_native_qt(g_file_dir_options, "native-qt", "Alias for --native-qt" XSTRINGIFY(QTDEF), {"native-qt"});
#  endif
#endif

#ifdef WITH_PROGRESS
  args::Group g_progress_options(ap_main, "Progress options:");
  ARG_T arg_pulsate(g_progress_options, "pulsate", "Pulsating progress bar", {"pulsate"});
  ARGI_T arg_multi(g_progress_options, "NUMBER", "Use 2 progress bars; the main bar, showing the overall progress, will reach 100% if the other bar has reached 100% after NUMBER iterations", {"multi"});
  ARGL_T arg_watch_pid(g_progress_options, "PID", "Process ID to watch", {"watch-pid"});
  ARG_T arg_auto_close(g_progress_options, "auto-close", "Dismiss the dialog when 100% has been reached", {"auto-close"});
  ARG_T arg_no_cancel(g_progress_options, "no-cancel", "Hide cancel button", {"no-cancel"});
#endif

#ifdef WITH_CHECKLIST
  args::Group g_checklist_options(ap_main, "Checklist options:");
  ARG_T arg_check_all(g_checklist_options, "check-all", "Start with all items selected", {"check-all"});
  ARG_T arg_return_value(g_checklist_options, "return-value", "Return list of selected items instead of a \"TRUE|FALSE\" list", {"return-value"});
#endif

#if defined(WITH_RADIOLIST) || defined(WITH_DROPDOWN)
  args::Group g_radiolist_dropdown_options(ap_main,
#if defined(WITH_RADIOLIST) && !defined(WITH_DROPDOWN)
    "Radiolist options:");
#elif !defined(WITH_RADIOLIST) && defined(WITH_DROPDOWN)
    "Dropdown options:");
#else
    "Radiolist/dropdown options:");
#endif
  ARG_T arg_return_number(g_radiolist_dropdown_options, "return-number", "Return selected entry number instead of label text", {"return-number"});
#endif

#if defined(WITH_CALENDAR) || defined(WITH_DATE)
  args::Group g_calendar_options(ap_main,
#if defined(WITH_CALENDAR) && !defined(WITH_DATE)
    "Calendar options:");
#elif !defined(WITH_CALENDAR) && defined(WITH_DATE)
    "Date options:");
#else
    "Calendar/date options:");
#endif
  ARGS_T arg_format(g_calendar_options, "FORMAT",
                    "Set a custom output format; interpreted sequences for FORMAT are:\n"
                    "(using the date 2006-01-08)\n"
                    "d   day (8)\n"
                    "D   day (08)\n"
                    "m   month (1)\n"
                    "M   month (01)\n"
                    "y   year (06)\n"
                    "Y   year (2006)\n"
                    "j   day of the year (8)\n"
                    "J   day of the year (008)\n"
                    "W   weekday name (Sunday)\n"
                    "w   weekday name (Sun)\n"
                    "n   ISO 8601 week number (1)\n"
                    "N   ISO 8601 week number (01)\n"
                    "B   month name (January)\n"
                    "b   month name (Jan)\n"
                    "u   day of the week, Monday being 1 (7)\n"
                    "\\n  newline character\n"
                    "\\t  tab character",
                    {"format"});
#endif

  args::Group g_scale_options(ap_main, "Scale options:\n(VALUE can be float point or integer)");
  ARGD_T arg_value(g_scale_options, "VALUE", "Set initial value", {"value"});
  ARGD_T arg_min_value(g_scale_options, "VALUE", "Set minimum value", {"min-value"});
  ARGD_T arg_max_value(g_scale_options, "VALUE", "Set maximum value", {"max-value"});
  ARGD_T arg_step(g_scale_options, "VALUE", "Set step size", {"step"});

#ifdef WITH_TEXTINFO
  args::Group g_text_info_options(ap_main, "Text information options:");
  ARGS_T arg_checkbox(g_text_info_options, "TEXT", "Enable an \"I read and agree\" checkbox", {"checkbox"});
  ARG_T arg_auto_scroll(g_text_info_options, "auto-scroll", "Always scroll to the bottom of the text", {"auto-scroll"});
#endif

#ifdef WITH_NOTIFY
  args::Group g_notification_options(ap_main, "Notification options:");
  ARGI_T arg_timeout(g_notification_options, "SECONDS", "Set the timeout value for the notification in seconds (may be ignored by some desktop environments)", {"timout"});
  ARGS_T arg_notify_icon(g_notification_options, "PATH", "Set the icon for the notification box", {"notify-icon"});
#endif

  try
  {
    ap_main.ParseCLI(argc, argv);
  }
  catch (args::Help)
  {
    std::cout << ap_main << "  using FLTK version " << get_fltk_version() << " - http://www.fltk.org\n\n"
      << "  https://github.com/darealshinji/fltk-dialog\n" << std::endl;
    return 0;
  }
  catch (args::ParseError e)
  {
    std::cerr << e.what() << std::endl;
    std::cerr << "See `" << argv[0] << " --help' for more information" << std::endl;
    return 1;
  }
  catch (args::ValidationError e)
  {
    std::cerr << e.what() << std::endl;
    std::cerr << "See `" << argv[0] << " --help' for more information" << std::endl;
    return 1;
  }

  if (arg_version)
  {
    std::cout << "using FLTK version " << get_fltk_version() << " - http://www.fltk.org" << std::endl;
    return 0;
  }

  int dialog_count = 0;  /* check if two or more dialog options were specified */
  int dialog = DIALOG_MESSAGE;  /* default message type */

  if (arg_about)
  {
    dialog = DIALOG_ABOUT;
    dialog_count++;
  }

  if (arg_no_escape)
  {
    Fl::add_handler(esc_handler);
  }

  window_decoration = arg_undecorated ? false : true;
  window_taskbar = arg_skip_taskbar ? false : true;

  const char *scheme = "default";
  const char *but_alt = NULL;

  GETCSTR(scheme, arg_scheme);
  GETCSTR(msg, arg_text);
  GETCSTR(title, arg_title);
  GETCSTR(fl_ok, arg_ok_label);
  GETCSTR(fl_close, arg_close_label);
  GETCSTR(fl_cancel, arg_cancel_label);
  GETCSTR(fl_yes, arg_yes_label);
  GETCSTR(fl_no, arg_no_label);
  GETCSTR(but_alt, arg_alt_label);

  if (arg_separator)
  {
    separator_s = args::get(arg_separator).substr(0,1);
    separator = separator_s.c_str()[0];
  }

  GETVAL(override_w, arg_width);
  GETVAL(override_h, arg_height);
  GETVAL(override_x, arg_posx);
  GETVAL(override_y, arg_posy);

  if (arg_geometry)
  {
    std::vector<std::string> v, v_wh;
    std::string s = args::get(arg_geometry);
    split(s, '+', v);

    if (v.size() != 3)
    {
      std::cerr << argv[0] << ": --geometry=WxH+X+Y: wrong format" << std::endl;
      return 1;
    }

    split(v[0], 'x', v_wh);
    if (v_wh.size() != 2)
    {
      std::cerr << argv[0] << ": --geometry=WxH+X+Y: wrong format" << std::endl;
      return 1;
    }

    STRINGTOINT(v[1], override_x, "--geometry=WxH+X+Y -> X");
    STRINGTOINT(v[2], override_y, "--geometry=WxH+X+Y -> Y");
    STRINGTOINT(v_wh[0], override_w, "--geometry=WxH+X+Y -> W");
    STRINGTOINT(v_wh[1], override_h, "--geometry=WxH+X+Y -> H");
  }

  resizable = arg_fixed ? false : true;
  position_center = arg_center ? true : false;

#ifdef WITH_DND
  if (arg_dnd) { dialog = DIALOG_DND; dialog_count++; }
#endif

#ifdef WITH_HTML
  const char *html = NULL;
  if (arg_html)
  {
    dialog = DIALOG_HTML;
    html = args::get(arg_html).c_str();
    dialog_count++;
  }
#endif

  if (arg_message) { dialog = DIALOG_MESSAGE; dialog_count++; }
  if (arg_warning) { dialog = DIALOG_WARNING; dialog_count++; }
  if (arg_question) { dialog = DIALOG_QUESTION; dialog_count++; }
  if (arg_entry) { dialog = DIALOG_INPUT; dialog_count++; }
  if (arg_password) { dialog = DIALOG_PASSWORD; dialog_count++; }

  bool with_icon_box = arg_no_symbol ? false : true;

#ifdef WITH_FILE
  if (arg_file) { dialog = DIALOG_FILE_CHOOSER; dialog_count++; }
  if (arg_directory) { dialog = DIALOG_DIR_CHOOSER; dialog_count++; }

# ifdef WITH_NATIVE_FILE_CHOOSER
  int native_count = 0;

  bool native = arg_native ? true : false;
  if (native) { native_count++; }

# ifdef HAVE_QT
  bool native_gtk = arg_native_gtk ? true : false;
  if (native_gtk) { native_count++; }

# ifdef HAVE_QT4
  bool native_qt4 = arg_native_qt4 ? true : false;
  if (native_qt4) { native_count++; }
# endif

# ifdef HAVE_QT5
  bool native_qt5 = arg_native_qt5 ? true : false;
  if (native_qt5) { native_count++; }
# endif

  if (arg_native_qt)
  {
# if QTDEF == 5
    native_qt5 = true;
# else
    native_qt4 = true;
# endif
    native_count++;
  }
# endif  /* HAVE_QT */
# endif  /* WITH_NATIVE_FILE_CHOOSER */
#endif  /* WITH_FILE */

#ifdef WITH_COLOR
  if (arg_color) { dialog = DIALOG_COLOR; dialog_count++; }
#endif

#ifdef WITH_NOTIFY
  int timeout = 5;
  const char *notify_icon = NULL;
  if (arg_notification) { dialog = DIALOG_NOTIFY; dialog_count++; }
  GETVAL(timeout, arg_timeout);
  GETCSTR(notify_icon, arg_notify_icon);
#endif

#ifdef WITH_PROGRESS
  int multi = 1;
  long kill_pid = -1;

  bool pulsate = arg_pulsate ? true : false;
  bool autoclose = arg_auto_close ? true : false;
  bool hide_cancel = arg_no_cancel ? true : false;

  if (arg_progress) { dialog = DIALOG_PROGRESS; dialog_count++; }

  GETVAL(kill_pid, arg_watch_pid);
  GETVAL(multi, arg_multi);

  if (arg_multi)
  {
    multi = (multi > 1) ? multi : 1;
  }
#endif

  if (arg_scale) { dialog = DIALOG_SCALE; dialog_count++; }

  GETVAL(scale_min, arg_min_value);
  scale_init = scale_min;

  GETVAL(scale_max, arg_max_value);
  GETVAL(scale_init, arg_value);
  GETVAL(scale_step, arg_step);

  if (scale_step < 1)
  {
    std::cerr << argv[0] << ": error `--step': value cannot be negative or zero" << std::endl;
    return 1;
  }

#ifdef WITH_CHECKLIST
  std::string checklist_options = "";
  GETVAL(checklist_options, arg_checklist);
  if (arg_checklist) { dialog = DIALOG_CHECKLIST; dialog_count++; }
  bool check_all = arg_check_all ? true : false;
  bool return_value = arg_return_value ? true : false;
#endif

#ifdef WITH_RADIOLIST
  std::string radiolist_options = "";
  GETVAL(radiolist_options, arg_radiolist);
  if (arg_radiolist) { dialog = DIALOG_RADIOLIST; dialog_count++; }
#endif

#ifdef WITH_DROPDOWN
  std::string dropdown_options = "";
  GETVAL(dropdown_options, arg_dropdown);
  if (arg_dropdown) { dialog = DIALOG_DROPDOWN; dialog_count++; }
#endif

#if defined(WITH_RADIOLIST) || defined(WITH_DROPDOWN)
  bool return_number = arg_return_number ? true : false;
#endif

#ifdef WITH_CALENDAR
  if (arg_calendar) { dialog = DIALOG_CALENDAR; dialog_count++; }
#endif

#ifdef WITH_DATE
  if (arg_date) { dialog = DIALOG_DATE; dialog_count++; }
#endif

#if defined(WITH_CALENDAR) || defined(WITH_DATE)
  std::string format = "";
  GETVAL(format, arg_format);
#endif

#ifdef WITH_TEXTINFO
  const char *checkbox = NULL;
  GETCSTR(checkbox, arg_checkbox);
  bool autoscroll = arg_auto_scroll ? true : false;
  if (arg_text_info) { dialog = DIALOG_TEXTINFO; dialog_count++; }
#endif

#ifdef WITH_FONT
  if (arg_font) { dialog = DIALOG_FONT; dialog_count++; }
#endif

#ifdef WITH_WINDOW_ICON
  if (arg_window_icon)
  {
    set_window_icon(args::get(arg_window_icon).c_str());
  }
#endif

  if (dialog_count >= 2)
  {
    std::cerr << argv[0] << ": two or more dialog options specified" << std::endl;
    return 1;
  }

  if (!with_icon_box && (dialog != DIALOG_MESSAGE && dialog != DIALOG_WARNING && dialog != DIALOG_QUESTION))
  {
    return use_only_with(argv[0], "--no-symbol", "--message, --warning or --question");
  }

#if defined(WITH_FILE) && defined(WITH_NATIVE_FILE_CHOOSER)
  if ((native || native_gtk || native_qt4 || native_qt5) && (dialog != DIALOG_FILE_CHOOSER && dialog != DIALOG_DIR_CHOOSER))
  {
    return use_only_with(argv[0], "--native/--native-gtk/--native-qt4/--native-qt5", "--file or --directory");
  }

  if (native_count >= 2)
  {
    std::cerr << argv[0] << ": two or more `--native' options specified" << std::endl;
    return 1;
  }
#endif

#ifdef WITH_NOTIFY
  if (dialog != DIALOG_NOTIFY)
  {
    if (arg_timeout)
    {
      return use_only_with(argv[0], "--timeout", "--notification");
    }

    if (notify_icon != NULL)
    {
      return use_only_with(argv[0], "--notify-icon", "--notification");
    }
  }
#endif

#ifdef WITH_PROGRESS
  if (dialog != DIALOG_PROGRESS)
  {
    if (pulsate)
    {
      return use_only_with(argv[0], "--pulsate", "--progress");
    }

    if (arg_multi)
    {
      return use_only_with(argv[0], "--multi", "--progress");
    }

    if (arg_watch_pid)
    {
      return use_only_with(argv[0], "--watch-pid", "--progress");
    }

    if (autoclose)
    {
      return use_only_with(argv[0], "--auto-close", "--progress");
    }

    if (hide_cancel)
    {
      return use_only_with(argv[0], "--no-cancel", "--progress");
    }
  }
  else if (dialog == DIALOG_PROGRESS && pulsate && arg_multi)
  {
    return use_only_with(argv[0], "--multi", "--progress, but not with --pulsate");
  }
#endif

  if ((arg_value || arg_min_value || arg_max_value || arg_step) && dialog != DIALOG_SCALE)
  {
    return use_only_with(argv[0], "--value/--min-value/--max-value/--step", "--scale");
  }

#ifdef WITH_CHECKLIST
  if (return_value && dialog != DIALOG_CHECKLIST)
  {
    return use_only_with(argv[0], "--return-value", "--checklist");
  }

  if (check_all && dialog != DIALOG_CHECKLIST)
  {
    return use_only_with(argv[0], "--check-all", "--checklist");
  }
#endif

#if defined(WITH_RADIOLIST) || defined(WITH_DROPDOWN)
  if (return_number && (dialog != DIALOG_RADIOLIST && dialog != DIALOG_DROPDOWN))
  {
    return use_only_with(argv[0], "--return-number",
#if defined(WITH_RADIOLIST) && !defined(WITH_DROPDOWN)
                         "--radiolist");
#elif !defined(WITH_RADIOLIST) && defined(WITH_DROPDOWN)
                         "--dropdown");
#else
                         "--radiolist or --dropdown");
#endif
  }
#endif

#if defined(WITH_CALENDAR) || defined(WITH_DATE)
  if (format != "" && (dialog != DIALOG_CALENDAR && dialog != DIALOG_DATE))
  {
    return use_only_with(argv[0], "--format",
#if defined(WITH_CALENDAR) && !defined(WITH_DATE)
                         "--calendar");
#elif !defined(WITH_CALENDAR) && defined(WITH_DATE)
                         "--date");
#else
                         "--calendar or --date");
#endif
  }
#endif

#ifdef WITH_TEXTINFO
  if (dialog != DIALOG_TEXTINFO && (autoscroll || checkbox))
  {
    return use_only_with(argv[0], "--auto-scroll/--checkbox", "--text-info");
  }
#endif

#if defined(WITH_HTML) || defined(WITH_DATE)
  /* keep fltk's '@' symbols enabled for HTML and Date dialogs */
  if (dialog != DIALOG_HTML && dialog != DIALOG_DATE)
  {
    Fl::set_labeltype(FL_NORMAL_LABEL, draw_cb, measure_cb);
  }
#endif

  if (STREQ("gtk", scheme))
  {
    scheme = "gtk+";
  }
  else if (STREQ("simple", scheme))
  {
    scheme = "none";
  }

  if (STREQ("default", scheme))
  {
    Fl::scheme(scheme_default);
  }
  else if (STREQ("none", scheme) || STREQ("gtk+", scheme) || STREQ("gleam", scheme) || STREQ("plastic", scheme))
  {
    Fl::scheme(scheme);
  }
  else
  {
    std::cerr << argv[0] << ": \"" << scheme << "\" is not a valid scheme!\n"
      "Available schemes are: default gtk+ gleam plastic simple" << std::endl;
    return 1;
  }

  if (!arg_no_system_colors)
  {
    Fl::get_system_colors();
  }

  switch (dialog)
  {
    case DIALOG_ABOUT:
      return about();
    case DIALOG_MESSAGE:
      return dialog_message(fl_close, NULL, NULL, MESSAGE_TYPE_INFO, with_icon_box);
    case DIALOG_WARNING:
      return dialog_message(fl_ok, fl_cancel, but_alt, MESSAGE_TYPE_WARNING, with_icon_box);
    case DIALOG_QUESTION:
      return dialog_message(fl_yes, fl_no, but_alt, MESSAGE_TYPE_QUESTION, with_icon_box);

#ifdef WITH_DND
    case DIALOG_DND:
      return dialog_dnd();
#endif

#ifdef WITH_FILE
    case DIALOG_FILE_CHOOSER:
#  ifdef WITH_NATIVE_FILE_CHOOSER
      if (native)
      {
        return dialog_native_file_chooser(FILE_CHOOSER, argc, argv);
      }
#    ifdef HAVE_QT
      else if (native_gtk)
      {
        return dialog_native_file_chooser_gtk(FILE_CHOOSER);
      }
#      ifdef HAVE_QT4
      else if (native_qt4)
      {
        return dialog_native_file_chooser_qt(4, FILE_CHOOSER, argc, argv);
      }
#      endif
#      ifdef HAVE_QT5
      else if (native_qt5)
      {
        return dialog_native_file_chooser_qt(5, FILE_CHOOSER, argc, argv);
      }
#      endif
#    endif  /* HAVE_QT */
      else
      {
        return dialog_file_chooser();
      }
#  else
      return dialog_file_chooser();
#  endif  /* WITH_NATIVE_FILE_CHOOSER */

    case DIALOG_DIR_CHOOSER:
#  ifdef WITH_NATIVE_FILE_CHOOSER
      if (native)
      {
        return dialog_native_file_chooser(DIR_CHOOSER, argc, argv);
      }
#    ifdef HAVE_QT
      else if (native_gtk)
      {
        return dialog_native_file_chooser_gtk(DIR_CHOOSER);
      }
#      ifdef HAVE_QT4
      else if (native_qt4)
      {
        return dialog_native_file_chooser_qt(4, DIR_CHOOSER, argc, argv);
      }
#      endif
#      ifdef HAVE_QT5
      else if (native_qt5)
      {
        return dialog_native_file_chooser_qt(5, DIR_CHOOSER, argc, argv);
      }
#      endif
#    endif  /* HAVE_QT */
      else
      {
        return dialog_dir_chooser();
      }
#  else
      return dialog_dir_chooser();
#  endif  /* WITH_NATIVE_FILE_CHOOSER */
#endif  /* WITH_FILE */

    case DIALOG_INPUT:
      return dialog_message(fl_ok, fl_cancel, but_alt, MESSAGE_TYPE_INPUT, false);

#ifdef WITH_HTML
    case DIALOG_HTML:
      return dialog_html_viewer(html);
#endif

    case DIALOG_PASSWORD:
      return dialog_message(fl_ok, fl_cancel, but_alt, MESSAGE_TYPE_PASSWORD, false);

#ifdef WITH_COLOR
    case DIALOG_COLOR:
      return dialog_color();
#endif

#ifdef WITH_NOTIFY
    case DIALOG_NOTIFY:
      return dialog_notify(argv[0], timeout, notify_icon);
#endif

#ifdef WITH_PROGRESS
    case DIALOG_PROGRESS:
      return dialog_progress(pulsate, multi, kill_pid, autoclose, hide_cancel);
#endif

    case DIALOG_SCALE:
      return dialog_message(fl_ok, fl_cancel, but_alt, MESSAGE_TYPE_SCALE, false);

#ifdef WITH_CHECKLIST
    case DIALOG_CHECKLIST:
      return dialog_checklist(checklist_options, return_value, check_all);
#endif

#ifdef WITH_RADIOLIST
    case DIALOG_RADIOLIST:
      return dialog_radiolist(radiolist_options, return_number);
#endif

#ifdef WITH_DROPDOWN
    case DIALOG_DROPDOWN:
      return dialog_dropdown(dropdown_options, return_number);
#endif

#ifdef WITH_CALENDAR
    case DIALOG_CALENDAR:
      return dialog_calendar(format);
#endif

#ifdef WITH_DATE
    case DIALOG_DATE:
      return dialog_date(format);
#endif

#ifdef WITH_FONT
    case DIALOG_FONT:
      return dialog_font();
#endif

#ifdef WITH_TEXTINFO
    case DIALOG_TEXTINFO:
      return dialog_textinfo(autoscroll, checkbox);
#endif

    /* should never be reached */
    default:
      std::cerr << argv[0] << ":\nmain(): error: unknown or unused dialog" << std::endl;
      return 1;
  }

  /* should never be reached */
  return 1;
}

