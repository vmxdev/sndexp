/*
 * $ `wx-config --cxx` -Wall -pedantic -Wextra `wx-config --cppflags` `wx-config --libs` -lcrypto instr.cc -o instr
 */
#include <wx/wx.h>
#include <wx/textctrl.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <wx/regex.h>

#include <openssl/md5.h>

class InstrApp: public wxApp
{
public:
	virtual bool OnInit();
};

class InstrFrame: public wxFrame
{
public:
	InstrFrame(const wxString& title, const wxPoint& pos,
		const wxSize& size);
private:
	void OnPlay(wxCommandEvent& event);
	void OnExit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);

	wxTextCtrl *instr, *notes;

	wxDECLARE_EVENT_TABLE();
};

enum
{
	ID_Play = 1
};

wxBEGIN_EVENT_TABLE(InstrFrame, wxFrame)
	EVT_MENU(ID_Play,    InstrFrame::OnPlay)
	EVT_MENU(wxID_EXIT,  InstrFrame::OnExit)
	EVT_MENU(wxID_ABOUT, InstrFrame::OnAbout)
wxEND_EVENT_TABLE()

wxIMPLEMENT_APP(InstrApp);

bool InstrApp::OnInit()
{
	InstrFrame *frame = new InstrFrame( "Instr",
		wxPoint(50, 50), wxSize(750, 400) );
	frame->Show( true );
	return true;
}

InstrFrame::InstrFrame(const wxString& title,
		const wxPoint& pos, const wxSize& size)
        : wxFrame(NULL, wxID_ANY, title, pos, size)
{
	wxMenu *menuFile = new wxMenu;
	menuFile->Append(ID_Play, "&Play...\tCtrl-P",
		"Compile & play");
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);
	wxMenu *menuHelp = new wxMenu;
	menuHelp->Append(wxID_ABOUT);
	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append( menuFile, "&File" );
	menuBar->Append( menuHelp, "&Help" );
	SetMenuBar( menuBar );

	instr = new wxTextCtrl(this, wxID_ANY,
		"tone = sin(t * 2 * M_PI * freq(note));",
		wxDefaultPosition,
		wxDefaultSize, wxTE_MULTILINE | wxTE_RICH2 | wxHSCROLL);

	wxFont fnt(14, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	instr->SetFont(fnt);

	notes = new wxTextCtrl(this, wxID_ANY,
		"add_note(tl, where, 28, notelen * 4, vol);",
		wxDefaultPosition,
		wxDefaultSize, wxTE_MULTILINE | wxTE_RICH2 | wxHSCROLL);

	notes->SetFont(fnt);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(instr, 1, wxEXPAND);
	sizer->Add(notes, 1, wxEXPAND);
	SetSizer(sizer);

	CreateStatusBar();
	SetStatusText( "Ready" );
}

void InstrFrame::OnExit(wxCommandEvent &)
{
	Close( true );
}

void InstrFrame::OnAbout(wxCommandEvent &)
{
	wxMessageBox("This is instr program",
		"About instr", wxOK | wxICON_INFORMATION);
}

wxString
calc_md5(const wxString &in)
{
	MD5_CTX md5;
	unsigned char output[50];
	char format_buffer[3];
	wxString res;

	MD5_Init(&md5);
	MD5_Update(&md5, in.c_str(), in.Length());
	MD5_Final(output, &md5);

	for (int i=0; i<MD5_DIGEST_LENGTH; i++) {
		sprintf(format_buffer, "%x", output[i]);
		res += format_buffer;
	}

	return res;
}


void InstrFrame::OnPlay(wxCommandEvent &)
{
	wxString tmpl_str, instr_str, notes_str;
	wxString out_name;

	SetStatusText("Compiling");

	wxFile tmpl("template.c");
	if (!tmpl.IsOpened()) {
		wxLogMessage("Can't open template.c");
		SetStatusText("Ready");
		return;
	}

	if (!tmpl.ReadAll(&tmpl_str)) {
		wxLogMessage("Can't read template.c");
		SetStatusText("Ready");
		return;
	}

	instr_str = instr->GetValue();
	notes_str = notes->GetValue();

	wxRegEx re_code_instr("%%CODE_INSTR%%");
	re_code_instr.ReplaceAll(&tmpl_str, instr_str);

	wxRegEx re_code_notes("%%CODE_NOTES%%");
	re_code_notes.ReplaceAll(&tmpl_str, notes_str);

	out_name = calc_md5(tmpl_str);
	if (!wxFileName::FileExists(out_name + ".exe")) {
		/* compile */
		wxFile out(out_name + ".c", wxFile::write);
		if (!out.Write(tmpl_str)) {
			wxLogMessage("Can't write to output file");
			SetStatusText("Ready");
			return;
		}

		wxString cmd = "cc -Wall -pedantic -Wextra " + out_name + ".c"
			+ " -o " + out_name + ".exe" + " -lm";

		wxExecute(cmd, wxEXEC_SYNC | wxEXEC_SHOW_CONSOLE);
		if (!wxFileName::FileExists(out_name + ".exe")) {
			wxLogMessage(wxString::Format("can't compile"));
			SetStatusText("Ready");
			return;
		}
	}

	wxString cmd = "./" + out_name + ".exe" + " | aplay -f S16_LE -r 44100 -c 2";
	wxShell(cmd);

	SetStatusText("Ready");
}

