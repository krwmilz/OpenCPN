/***************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  OpenCPN iENCToolbar
 * Author:   David Register
 *
 ***************************************************************************
 *   Copyright (C) 2017 by David S. Register                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 **************************************************************************/

#include "config.h"

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "toolbar.h"
#include "iENCToolbar.h"

#include "chart1.h"
#include "chcanv.h"
#include "s52s57.h"
#include "s52plib.h"
#include "pluginmanager.h"
#include "OCPNPlatform.h"
#include "chcanv.h"

extern s52plib *ps52plib;
extern MyFrame *gFrame;
extern OCPNPlatform *g_Platform;

extern wxImage LoadSVGIcon(wxString filename, int width, int height);

//---------------------------------------------------------------------------------------
//          iENCToolbar Implementation
//---------------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(iENCToolbar, ocpnFloatingToolbarDialog)
EVT_MENU(wxID_ANY, iENCToolbar::OnToolLeftClick)
EVT_TIMER(STATE_TIMER, iENCToolbar::StateTimerEvent)
EVT_MOUSE_EVENTS(iENCToolbar::MouseEvent)
END_EVENT_TABLE()

iENCToolbar::iENCToolbar(wxWindow *parent, wxPoint position, long orient,
                         float size_factor)
    : ocpnFloatingToolbarDialog(parent, position, orient, size_factor) {
  LoadToolBitmaps();

  wxSize a = m_bmMinimum.GetSize();
  m_ptoolbar->SetToolBitmapSize(a);

  m_pbmScratch = new wxBitmap(a.x, a.y);

  m_bmTemplate = m_bmRMinus;

  m_toolDensity =
      m_ptoolbar->AddTool(ID_DENSITY, _T("Density"), m_bmMinimum, m_bmMinimum);
  m_ptoolbar->AddTool(ID_RPLUS, _T("RangePlus"), m_bmRPlus, m_bmRPlus);
  m_ptoolbar->AddTool(ID_RMINUS, _T("RangeMinus"), m_bmRMinus, m_bmRMinus);

  SetCanToggleOrientation(false);
  EnableRolloverBitmaps(false);
  DisableTooltips();
  SetGrabberEnable(false);

  m_nDensity = 0;
  SetDensityToolBitmap(m_nDensity);

  // Realize() the toolbar
  Realize();
  Hide();

  SetDefaultPosition();
  Show(true);

  m_state_timer.SetOwner(this, STATE_TIMER);
  m_state_timer.Start(500, wxTIMER_CONTINUOUS);
}

iENCToolbar::~iENCToolbar() {}

void iENCToolbar::MouseEvent(wxMouseEvent &event) {
  if (event.IsButton()) gFrame->Raise();
}

void iENCToolbar::SetColorScheme(ColorScheme cs) {
  m_range = 0;  // Forcw a redraw of tools
  m_nDensity = -1;

  ocpnFloatingToolbarDialog::SetColorScheme(cs);
}

void iENCToolbar::LoadToolBitmaps() {
  wxString svgDir = g_Platform->GetSharedDataDir() + _T("uidata") +
                    wxFileName::GetPathSeparator();

  int w = 96;
  int h = 32;

  if (::wxFileExists(svgDir + _T("iENC_All.svg"))) {
    wxImage img = LoadSVGIcon(svgDir + _T("iENC_All.svg"), w, h);
    m_bmAll = wxBitmap(img);
    img = LoadSVGIcon(svgDir + _T("iENC_Minimum.svg"), w, h);
    m_bmMinimum = wxBitmap(img);
    img = LoadSVGIcon(svgDir + _T("iENC_Standard.svg"), w, h);
    m_bmStandard = wxBitmap(img);
    img = LoadSVGIcon(svgDir + _T("iENC_UserStd.svg"), w, h);
    m_bmUStd = wxBitmap(img);

    img = LoadSVGIcon(svgDir + _T("iENC_RPlus.svg"), w, h);
    m_bmRPlus = wxBitmap(img);
    img = LoadSVGIcon(svgDir + _T("iENC_RMinus.svg"), w, h);
    m_bmRMinus = wxBitmap(img);

  } else {
    wxLogMessage(_T("Cannot find iENC icons at: ") + svgDir);

    m_bmMinimum = wxBitmap(96, 32);
    m_bmStandard = wxBitmap(96, 32);
    ;
    m_bmAll = wxBitmap(96, 32);
    m_bmUStd = wxBitmap(96, 32);

    m_bmRPlus = wxBitmap(96, 32);
    m_bmRMinus = wxBitmap(96, 32);
  }
}

void iENCToolbar::OnToolLeftClick(wxCommandEvent &event) {
  int itemId = event.GetId();

  ChartCanvas *cc = gFrame->GetPrimaryCanvas();

  enum _DisCat nset = STANDARD;
  double range;

  switch (itemId) {
    case ID_DENSITY:

      if (++m_nDensity > 3) m_nDensity = 0;

      SetDensityToolBitmap(m_nDensity);
      m_ptoolbar->Refresh();

      switch (m_nDensity) {
        case 0:
          nset = DISPLAYBASE;
          break;
        case 1:
          nset = STANDARD;
          break;
        case 2:
          nset = OTHER;
          break;
        case 3:
          nset = MARINERS_STANDARD;
          break;
        default:
          nset = STANDARD;
          break;
      }

      gFrame->SetENCDisplayCategory(cc, nset);

      break;

    case ID_RMINUS:
      range = cc->GetCanvasRangeMeters();
      range = wxRound(range * 10) / 10.;

      if (range > 8000.) cc->SetCanvasRangeMeters(8000.);
      if (range > 4000.)
        cc->SetCanvasRangeMeters(4000.);
      else if (range > 2000.)
        cc->SetCanvasRangeMeters(2000.);
      else if (range > 1600.)
        cc->SetCanvasRangeMeters(1600.);
      else if (range > 1200.)
        cc->SetCanvasRangeMeters(1200.);
      else if (range > 800.)
        cc->SetCanvasRangeMeters(800.);
      else if (range > 500.)
        cc->SetCanvasRangeMeters(500.);
      else if (range > 300.)
        cc->SetCanvasRangeMeters(300.);

      break;

    case ID_RPLUS:
      range = cc->GetCanvasRangeMeters();
      range = wxRound(range * 10) / 10.;

      if (range < 300.)
        cc->SetCanvasRangeMeters(300.);
      else if (range < 500.)
        cc->SetCanvasRangeMeters(500.);
      else if (range < 800.)
        cc->SetCanvasRangeMeters(800.);
      else if (range < 1200.)
        cc->SetCanvasRangeMeters(1200.);
      else if (range < 1600.)
        cc->SetCanvasRangeMeters(1600.);
      else if (range < 2000.)
        cc->SetCanvasRangeMeters(2000.);
      else if (range < 4000.)
        cc->SetCanvasRangeMeters(4000.);
      else if (range < 8000.)
        cc->SetCanvasRangeMeters(8000.);

      break;

    default:
      break;
  }
}

void iENCToolbar::SetDensityToolBitmap(int nDensity) {
  int toolID = m_toolDensity->GetId();

  if (nDensity == 0)
    m_ptoolbar->SetToolBitmaps(ID_DENSITY, &m_bmMinimum, &m_bmMinimum);
  else if (nDensity == 1)
    m_ptoolbar->SetToolBitmaps(ID_DENSITY, &m_bmStandard, &m_bmStandard);
  else if (nDensity == 2)
    m_ptoolbar->SetToolBitmaps(ID_DENSITY, &m_bmAll, &m_bmAll);
  else if (nDensity == 3)
    m_ptoolbar->SetToolBitmaps(ID_DENSITY, &m_bmUStd, &m_bmUStd);
}

void iENCToolbar::StateTimerEvent(wxTimerEvent &event) {
  ChartCanvas *cc = gFrame->GetPrimaryCanvas();
  if (!cc) return;

  //  Keep the Density tool in sync
  if (ps52plib) {
    int nset = 1;

    switch (gFrame->GetPrimaryCanvas()->GetENCDisplayCategory()) {
      case (DISPLAYBASE):
        nset = 0;
        break;
      case (STANDARD):
        nset = 1;
        break;
      case (OTHER):
        nset = 2;
        break;
      case (MARINERS_STANDARD):
        nset = 3;
        break;
      default:
        nset = 1;
        break;
    }

    if (nset != m_nDensity) {
      if (nset < 3) {
        m_nDensity = nset;
        SetDensityToolBitmap(m_nDensity);

        m_ptoolbar->Refresh();
      }
    }
  }

  // Keep the Range annunciator updated

  if (cc) {
    double range = cc->GetCanvasRangeMeters();

    if (range != m_range) {
      m_range = range;

#if 0
            // This DOES NOT WORK on Mac
            //  None of it....

//            wxImage image = m_bmRMinus.ConvertToImage();
//            wxBitmap bmTemplate(image);

            // Get the template bitmap
            wxBitmap bmTemplate = m_bmRMinus;

            wxMask *mask = new wxMask(bmTemplate, wxColour(wxTRANSPARENT));
            bmTemplate.SetMask(mask);

            wxMemoryDC dct(bmTemplate);

            //  Make a deep copy by Blit
            wxMemoryDC dc;
            dc.SelectObject(*m_pbmScratch);
            dc.SetBackground(wxBrush(GetGlobalColor(_T("GREY2"))));
            dc.Clear();

            dc.Blit(0, 0, m_pbmScratch->GetWidth(), m_pbmScratch->GetHeight(), &dct, 0, 0, wxCOPY, true);

            dct.SelectObject(wxNullBitmap);
#else
      wxMemoryDC dc;
      dc.SelectObject(*m_pbmScratch);
      dc.SetBackground(wxBrush(GetGlobalColor(_T("GREY2"))));
      dc.Clear();
      dc.DrawBitmap(m_bmRMinus, wxPoint(0, 0));
#endif
      //  Render the range as text onto the template
      m_rangeFont = wxTheFontList->FindOrCreateFont(
          12, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

      wxString range_string;
      range_string.Printf(_T("%4.0fm"), range);

      dc.SetFont(*m_rangeFont);

      //  Select a font that will fit into the allow space.
      bool good = false;
      int target_points = 12;
      while (!good && (target_points > 4)) {
        int width, height;
        dc.GetTextExtent(range_string, &width, &height);
        if (width < 50) {
          good = true;
          break;
        } else {
          target_points--;
          m_rangeFont = wxTheFontList->FindOrCreateFont(
              target_points, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL,
              wxFONTWEIGHT_NORMAL);
          dc.SetFont(*m_rangeFont);
        }
      }

      dc.SetTextForeground(*wxBLACK);
      dc.SetBackgroundMode(wxTRANSPARENT);

      dc.DrawText(range_string, 42, 8);

      dc.SelectObject(wxNullBitmap);

      m_ptoolbar->SetToolBitmaps(ID_RMINUS, m_pbmScratch, m_pbmScratch);

      m_ptoolbar->Refresh();
    }
  }
}
