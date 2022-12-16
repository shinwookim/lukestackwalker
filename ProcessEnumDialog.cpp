#include <tchar.h>

#include <wx/dialog.h>
#include <wx/button.h>
#include <wx/textCtrl.h>
#include <wx/listBox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>

#include "ProcessEnumDialog.h"
#include <wx/config.h>
#include <wx/confbase.h>


#include <Tlhelp32.h>
#include <wx/msgdlg.h>

enum {
  ID_REFRESH = wxID_HIGHEST + 1,  
  ID_SORT,
  ID_ALLITEMS_LB,
};




ProcessEnumDialog::ProcessEnumDialog(wxWindow *parent) 
  : wxDialog(parent, wxID_ANY, wxString(_T("Select Process to Profile")))

{  
  wxBoxSizer *sizerTop = new wxBoxSizer(wxVERTICAL);
  SetSizer(sizerTop);

  wxStaticText *static1 = new wxStaticText(this, wxID_ANY, "Currently running processes:");
  sizerTop->Add(static1, 0, wxEXPAND|wxALL, 5 );

  m_items = new wxListBox(this, ID_ALLITEMS_LB, wxDefaultPosition, wxSize(300, 400));
  sizerTop->Add(m_items, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

  m_sortByNameCheckBox = new wxCheckBox(this, ID_SORT, "Sort by process name");  

  sizerTop->Add(m_sortByNameCheckBox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 5 );
  m_sortByNameCheckBox->Set3StateValue(wxCHK_CHECKED);

  wxBoxSizer *sizerBottomRow = new wxBoxSizer(wxHORIZONTAL);      

  m_refreshButton = new wxButton(this, ID_REFRESH, _T("&Refresh list"));  
  sizerBottomRow->Add(m_refreshButton, 0, wxEXPAND|wxALL, 5 );
  
  m_okButton = new wxButton(this, wxID_OK, _T("&OK"));
  sizerBottomRow->Add(m_okButton, 0, wxEXPAND|wxALL, 5 );
  m_cancelButton = new wxButton(this, wxID_CANCEL, _T("&Cancel"));
  sizerBottomRow->Add(m_cancelButton, 0, wxEXPAND|wxALL, 5 );
  
  sizerTop->Add(sizerBottomRow, 0, wxTOP|wxBOTTOM|wxALIGN_RIGHT, 5);

  sizerTop->SetSizeHints(this);
  sizerTop->Fit(this);

  m_cancelButton->SetFocus();
  m_cancelButton->SetDefault();
  RefreshProcesses();

}

BEGIN_EVENT_TABLE(ProcessEnumDialog, wxDialog)
  EVT_BUTTON(wxID_OK, ProcessEnumDialog::OnOk)
  EVT_BUTTON(ID_REFRESH, ProcessEnumDialog::OnRefresh)
  EVT_CHECKBOX(ID_SORT, ProcessEnumDialog::OnRefresh) 
END_EVENT_TABLE()


void ProcessEnumDialog::OnOk(wxCommandEvent& ev) {    
  wxString str = m_items->GetStringSelection();
  if (str.empty()) {
    return;
  }

  wxString procName = str.AfterFirst(':');
  procName = procName.AfterFirst(' ');  
  wxConfigBase::Get()->Write(_T("LastProcessName"), procName);  
  
  
  sscanf(str.c_str(), "%x", &m_processId);
  if (m_processId == GetCurrentProcessId()) {
    wxMessageBox(_T("Cannot profile myself.\nYou can profile another copy of Luke Stackwalker, though."),
                     _T("Error"),
                     wxOK | wxICON_INFORMATION, this);    
    ev.StopPropagation();
    return;
  }
  ev.Skip();
}

void ProcessEnumDialog::OnRefresh(wxCommandEvent& ev) {  
  RefreshProcesses();
  ev.Skip();
}

void ProcessEnumDialog::RefreshProcesses() {
  m_items->Freeze();
  m_items->Clear();
  const bool bSort = m_sortByNameCheckBox->IsChecked();

  HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  PROCESSENTRY32 pe;
  memset(&pe, 0, sizeof(pe));
  pe.dwSize = sizeof(pe);
  BOOL bRet = Process32First(hSnap, &pe);
  while (bRet) {
    wchar_t buf[256];
    wsprintf(buf, L"%04x : %s", pe.th32ProcessID, pe.szExeFile);
    bool bInserted = false;
    if (bSort) {
      unsigned int i = 0;      
      for (i = 0; i < m_items->GetCount(); i++) {
        wxString item = m_items->GetString(i);
        item = item.AfterFirst(':');
        item = item.AfterFirst(' ');
        int cmp = _wcsicmp(pe.szExeFile, item.c_str());  
        if (cmp < 0) {
          bInserted = true;
          m_items->Insert(buf, i);
          break;
        }
      }
    } 
    if (!bInserted)
      m_items->Append(buf);
    OutputDebugString(L"\n");
    memset(&pe, 0, sizeof(pe));
    pe.dwSize = sizeof(pe);
    bRet = Process32Next(hSnap, &pe); 
  }
  CloseHandle(hSnap);
  auto procName = wxConfigBase::Get()->Read(_T("LastProcessName"), "");
  for (unsigned int i = 0; i < m_items->GetCount(); i++) {
    wxString item = m_items->GetString(i);
    item = item.AfterFirst(':');
    item = item.AfterFirst(' ');
    if (item == procName) {
      m_items->SetSelection(i);
      m_items->EnsureVisible(i);
      break;
    }
  }
  
  m_items->Thaw();
}
