/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016, djcj <djcj@gmx.de>
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

#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Valuator.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Window.H>

#include <iostream>
#include <string>
#include <string.h>

#include "fltk-dialog.hpp"

static Fl_Window *slider_win;
static double slidval_round;

static void slider_cb(Fl_Widget *w)
{
  slidval_round = ((Fl_Valuator *)w)->value();
}

static void slider_close_cb(Fl_Widget *, long p)
{
  slider_win->hide();
  ret = (int) p;
}

int dialog_fl_value_slider(double min,
                           double max,
                           double step,
                           double val)
{
  Fl_Group         *g;
  Fl_Box           *dummy, *box;
  Fl_Value_Slider  *slider;
  Fl_Return_Button *but_ok;
  Fl_Button        *but_cancel;

  std::string s;
  int textlines = 1;

  if (min == -1)
  {
    min = 0;
  }
  if (max == -1)
  {
    max = 100;
  }
  if (step == -1)
  {
    step = 1;
  }
  if (val == -1)
  {
    val = 0;
  }

  if (msg == NULL)
  {
    s = "Simple FLTK scaler";
  }
  else
  {
    s = translate(msg);
    for (size_t i = 0; i < strlen(msg); i++)
    {
      if (msg[i] == '\n')
      {
        textlines++;
      }
    }
  }

  if (title == NULL)
  {
    title = (char *)"FLTK slider window";
  }

  int box_h = (textlines * 18) + 20;
  int mod_h = box_h + 78;

  slider_win = new Fl_Window(320, mod_h, title);
  slider_win->callback(slider_close_cb, 1);
  {
    g = new Fl_Group(0, 0, 320, mod_h);
    {
      box = new Fl_Box(0, 0, 10, box_h, s.c_str());
      box->box(FL_NO_BOX);
      box->align(FL_ALIGN_RIGHT);

      slider = new Fl_Value_Slider(10, box_h, 300, 30, NULL);
      slider->type(FL_HOR_NICE_SLIDER);
      slider->box(FL_FLAT_BOX);
      slider->minimum(min);
      slider->maximum(max);
      slider->step(step);
      slidval_round = slider->round(val);
      slider->value(slidval_round);
      slider->callback(slider_cb);

      dummy = new Fl_Box(119, mod_h - 38, 1, 1);
      dummy->box(FL_NO_BOX);
      but_ok = new Fl_Return_Button(120, mod_h - 34, 90, 26, fl_ok);
      but_ok->callback(slider_close_cb, 0);
      but_cancel = new Fl_Button(220, mod_h - 34, 90, 26, fl_cancel);
      but_cancel->callback(slider_close_cb, 1);
    }
    g->resizable(dummy);
    g->end();
  }
  set_size(slider_win, g);
  set_position(slider_win);
  slider_win->end();
  slider_win->show();
  Fl::run();

  if (ret == 0)
  {
    std::cout << slidval_round << std::endl;
  }
  return ret;
}

