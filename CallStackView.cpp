
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include "callstackview.h"
#include <wx/dcclient.h>
#include "sampledata.h"
#include <wx/filename.h>
#include <wx/mstream.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>


BEGIN_EVENT_TABLE(CallStackView, wxScrolledWindow)
EVT_LEFT_UP (CallStackView::OnLeftButtonUp)
END_EVENT_TABLE()

CallStackView::CallStackView(wxNotebook *parent, ProfilerSettings *pSettings) :
    wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxSize(100, 100),
                     wxHSCROLL | wxVSCROLL | wxNO_FULL_REPAINT_ON_RESIZE),
    m_pSettings(pSettings)  
{
  m_parent = parent;
  m_bShowSamplesAsSampleCounts = true;
  m_font = wxFont(10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Arial");
  wxASSERT(m_font.IsOk());
  m_gvc = gvContext();
  m_fs = 0;
  m_zoom = 1.0; 
  m_graph = 0;
  enum {MAXWIDTH=2};
  enum {RSTART = 250, GSTART = 239, BSTART = 50, 
         REND = 250, GEND= 0, BEND = 0,
         RSTEP = (REND - RSTART)/NPENS,
         GSTEP = (GEND - GSTART)/NPENS,
         BSTEP = (BEND - BSTART)/NPENS};
  for (int i = 0 ; i < NPENS; i++) {
    const int w = (MAXWIDTH*(i+i))/NPENS;
    m_pens[i] = new wxPen(wxColour(0, 0, 0), w?w:1, w? wxPENSTYLE_SOLID : wxPENSTYLE_DOT);
    m_brushes[i] = new wxBrush(wxColour(RSTART + RSTEP*i, GSTART + GSTEP*i, BSTART + BSTEP*i));
  }
  m_pcb = 0;
}

CallStackView::~CallStackView() {
  if (m_graph) {
    gvFreeLayout(m_gvc, m_graph);
    agclose(m_graph);
    gvFreeContext(m_gvc);
  }
  for (int i = 0 ; i < NPENS; i++) {
    delete m_pens[i];
    delete m_brushes[i];
  }
}


void CallStackView::DoGraph(FunctionSample *fs, bool bSkipPCInUnknownModules) {
  if (m_graph) {
    gvFreeLayout(m_gvc, m_graph);
    agclose(m_graph);
  }

  /* Create a simple digraph */
  m_graph = agopen("graph", Agdirected, 0);
  

  for (std::list<Caller>::iterator nodeit = fs->m_callgraph.begin(); nodeit != fs->m_callgraph.end(); ++nodeit) {
      if (bSkipPCInUnknownModules && !nodeit->m_functionSample->m_moduleName.length())
        continue;
      char name[1024] = { 0 };
      
      const int totalSamples = g_displayedSampleInfo->m_totalSamples - g_displayedSampleInfo->GetIgnoredSamples();

      if (nodeit == fs->m_callgraph.begin()) {
        char samples[64];
        if (m_bShowSamplesAsSampleCounts) {
          sprintf(samples, "%d", fs->m_sampleCount);
        } else {
          double val = (100.0 * (double)(fs->m_sampleCount) / totalSamples);
          sprintf(samples, "%0.1lf%%", val);          
        }
        _snprintf_s(name, sizeof(name), _TRUNCATE, "%s\nsamples:%s", (const char *)m_pSettings->DoAbbreviations(fs->m_functionName).c_str(), samples);
      } else {
        char samples[64];
        if (m_bShowSamplesAsSampleCounts) {
          sprintf(samples, "%d", nodeit->m_sampleCount);
        } else {
          const double val = (100.0 * (double)(nodeit->m_sampleCount) / totalSamples);
          sprintf(samples, "%0.1lf%%", val);          
        }

        wxFileName fn(nodeit->m_functionSample->m_fileName); 
        if (!nodeit->m_functionSample->m_fileName.length()) {
         _snprintf_s(name, sizeof(name), _TRUNCATE, "%s\n%ls\nline:%d samples:%s", (const char *)m_pSettings->DoAbbreviations(nodeit->m_functionSample->m_functionName).c_str(),
                  nodeit->m_functionSample->m_moduleName.c_str(), nodeit->m_lineNumber, samples);
        } else {
         _snprintf_s(name, sizeof(name), _TRUNCATE, "%s\n%s.%s\nline:%d samples:%s", (const char *)m_pSettings->DoAbbreviations(nodeit->m_functionSample->m_functionName).c_str(),
                  (const char *)fn.GetName().c_str(), (const char*)fn.GetExt().c_str(), nodeit->m_lineNumber, samples);
        }
      }      
      nodeit->m_graphNode = agnode(m_graph, name, true);
      agsafeset(nodeit->m_graphNode, "fontsize", "18", "");
      agsafeset(nodeit->m_graphNode, "fontname", "Arial", "");
      agsafeset(nodeit->m_graphNode, "shape", "box", "");      
  }

 
  for (auto nodeit = fs->m_callgraph.begin(); 
       nodeit != fs->m_callgraph.end(); ++nodeit) {
      if (bSkipPCInUnknownModules && !nodeit->m_functionSample->m_moduleName.length())
        continue;
      for (std::list<Call>::iterator edgeit = nodeit->m_callsFromHere.begin();
        edgeit != nodeit->m_callsFromHere.end(); ++edgeit) {
          if (edgeit->m_target->m_graphNode) {
            edgeit->m_graphEdge = agedge(m_graph, nodeit->m_graphNode, edgeit->m_target->m_graphNode, "", true);
          }
      }
  }

  /* Use the directed graph layout engine */
  gvLayout(m_gvc, m_graph, "dot");

  gvRender(m_gvc, m_graph, "dot", 0);

  // the above 2 functions will fail if all the plugin dll's can't be loaded or if config6 file listing the plugings does not exist



}

void CallStackView::ShowCallstackToFunction(const wchar_t *funcName, bool bSkipPCInUnknownModules) {  
  m_funcName = funcName;
  m_fs = 0;

  
  if (g_displayedSampleInfo) {
    const auto fsit = g_displayedSampleInfo->m_functionSamples.find(m_funcName);
    if (fsit != g_displayedSampleInfo->m_functionSamples.end()) {
      FunctionSample *fs = &fsit->second;
      DoGraph(fs, bSkipPCInUnknownModules);
      m_fs = fs;
    }
  }

  
  for (int i = 0; i < (int)m_parent->GetPageCount(); i++) {
    if (m_fs && m_parent->GetPage(i) == this) {
      m_parent->ChangeSelection(i);
      char buf[512];
      _snprintf_s(buf, sizeof(buf), _TRUNCATE, "Call Graph of %S", funcName);
      m_parent->SetPageText(i, buf);
      break;
    }
    if (!m_fs && m_parent->GetPage(i) == this) {
      m_parent->SetPageText(i, "Call Graph");
    }
    if (!m_fs && m_parent->GetPage(i) != this) {
      m_parent->ChangeSelection(i);      
    }
  }
  
  Scroll(0, 0);
  if (m_fs)
    Refresh();
  
}

static constexpr  float xscale = 0.76f;

void CallStackView::OnDraw(wxDC &dc) {
  

  if (!m_fs || !m_graph) {
    return;
  }
  dc.SetUserScale(m_zoom, m_zoom);

  
  dc.SetFont(m_font);

  wxSize windowSize = GetClientSize();
  windowSize.x = windowSize.x / m_zoom;
  windowSize.y = windowSize.y / m_zoom;
  wxSize maxPoint(GD_bb(m_graph).UR.x*xscale, GD_bb(m_graph).UR.y);
  if (maxPoint.x < windowSize.x)
    maxPoint.x = windowSize.x;
  if (maxPoint.y < windowSize.y)
    maxPoint.y = windowSize.y;
  dc.SetPen(*wxWHITE_PEN);
  dc.DrawRectangle(-1, -1, maxPoint.x + 30, maxPoint.y + 30);

  
  // draw graph edges
  for (std::list<Caller>::iterator nodeit = m_fs->m_callgraph.begin(); 
    nodeit != m_fs->m_callgraph.end(); ++nodeit) {
      for (std::list<Call>::iterator edgeit = nodeit->m_callsFromHere.begin();
        edgeit != nodeit->m_callsFromHere.end(); ++edgeit) {
          int nPen = ((NPENS - 1) * edgeit->m_count) / (m_fs->m_sampleCount?m_fs->m_sampleCount:1);
          if (nPen >= NPENS) {
            nPen = NPENS - 1;
          }
          dc.SetPen(*m_pens[nPen]);            
          dc.SetBrush(*m_brushes[nPen]);
          if (edgeit->m_graphEdge && ED_spl(edgeit->m_graphEdge)) {
            Agedge_t * const edge = edgeit->m_graphEdge;
            const auto nSplines = ED_spl(edge)->size;
            for (int spl = 0; spl < nSplines; spl++) {
              const int n = ED_spl(edge)->list[spl].size;
              wxPoint* pt = new wxPoint[n];
              for (int i = 0; i < n; i++) {
                pt[i].x = ED_spl(edge)->list[spl].list[i].x * xscale;
                pt[i].y = ED_spl(edge)->list[spl].list[i].y;
              }
              dc.DrawSpline(n, pt);

              if (ED_spl(edge)->list->eflag) {
                const wxPoint ep(ED_spl(edge)->list->ep.x * xscale, ED_spl(edge)->list->ep.y);
                const wxPoint sp = pt[n - 1];
                wxPoint delta = sp - ep;
                const int tmp = delta.x;
                delta.x = delta.y;
                delta.y = tmp;
                delta.x /= -2;
                delta.y /= 2;
                wxPoint arrow[3];
                arrow[0] = ep;
                arrow[1] = sp + delta;
                arrow[2] = sp - delta;
                dc.DrawPolygon(3, arrow);
              }
              delete[] pt;
            }
          }
      }
  }
  
  // draw graph nodes
  for (std::list<Caller>::iterator nodeit = m_fs->m_callgraph.begin(); 
    nodeit != m_fs->m_callgraph.end(); ++nodeit) {
      int nPen = ((NPENS - 1)* nodeit->m_sampleCount) / (m_fs->m_sampleCount?m_fs->m_sampleCount:1);
      if (nPen >= NPENS) {
        nPen = NPENS - 1;
      }
      dc.SetPen(*wxBLACK_PEN);
      dc.SetBrush(*m_brushes[nPen]);
      if (!nodeit->m_graphNode)
        continue;
      Agnode_t *node = nodeit->m_graphNode;
      const auto& bb = ND_bb(node);
      dc.DrawRectangle(bb.LL.x*xscale, bb.LL.y,
        (bb.UR.x - bb.LL.x)*xscale,
        bb.UR.y - bb.LL.y);

     
      if (ND_label(node) && ND_label(node)->text) {
        wxString txt = ND_label(node)->text;
        const int x = (bb.UR.x + bb.LL.x)*xscale / 2; // center of ellipse in x
        int y = ND_label(node)->pos.y;
        do {           
          wxString str2 = txt.BeforeFirst('\n');
          const wxSize sz = dc.GetTextExtent(str2);
          int yOffs = sz.y;
          if (nodeit != m_fs->m_callgraph.begin()) {
            yOffs += sz.y / 2;
          }
          dc.DrawText(str2, x - sz.x / 2, y -  yOffs);
          y += sz.y;
          txt = txt.AfterFirst('\n');
        } while (txt.length());
      }
  }


  SetVirtualSize(maxPoint.x * m_zoom, maxPoint.y * m_zoom);
  SetScrollRate(20, 20);
}

void CallStackView::OnLeftButtonUp(wxMouseEvent &evt) {
  wxClientDC dc(this);
  PrepareDC(dc);    
  dc.SetUserScale(m_zoom, m_zoom);
  const wxPoint pt(evt.GetLogicalPosition(dc));
  if (!m_fs || !m_pcb) {
    return;
  }
  for (std::list<Caller>::iterator nodeit = m_fs->m_callgraph.begin(); 
    nodeit != m_fs->m_callgraph.end(); ++nodeit) {
      if (!nodeit->m_graphNode)
        continue;
      Agnode_t *node = nodeit->m_graphNode;
      const auto bb = ND_bb(node);
      const wxRect nodeRect(bb.LL.x*xscale, bb.LL.y,
        (bb.UR.x - bb.LL.x)*xscale,
         bb.UR.y - bb.LL.y);
      if (nodeRect.Contains(pt)) {
        m_pcb->OnClickCaller(&(*nodeit));
      }
  }
}

void CallStackView::SetZoom(double zoom) {
  m_zoom = zoom;
  Refresh();
}
