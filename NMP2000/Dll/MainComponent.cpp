#include "../JuceLibraryCode/JuceHeader.h"
#include "Markup.h"
#include <Shlobj.h>
#include "Variables.h"
#include <Windows.h>

Component* createMainContentComponent();
typedef void(__stdcall * TAriadniCallBackFunc)(int Status, double SettingCode);
int ArStatus = -1;
int ExitStatus = -1;
int EnableSCan = -1;
bool VisibilityState = false;

TAriadniCallBackFunc CallBack;
bool PlayTones = false;
#pragma once
enum    TDestChannels {
	BeamDeflectUp = 0x01,
	BeamDeflectLo = 0x02,
	ImageDeflectUp = 0x04,
	ImageDeflectLo = 0x08,
	CameraShutter = 0x10,
};

#pragma pack(push, 1)

struct TScanData {
	double BDUX;
	double BDUY;
	double BDLX;
	double BDLY;
	double IDUX;
	double IDUY;
	// double IDLX;
	// double IDLY;
	double Channel9;
	double Shutter;
	int Time;
};

void setRuster(double Table[2][512])
{
	int oldj = 0;
	if (SR == 48000.0)
	{
		for (int i = 0; i<512; i++)
		{
			RusterArrayX[i] = Table[0][i];
			RusterArrayX2[i] = Table[0][i];
			RusterArrayX3[i] = Table[0][i];
		}
		for (int i = 0; i<512; i++)
		{
			RusterArrayY[i] = Table[1][i];
			RusterArrayY2[i] = Table[1][i];
			RusterArrayY3[i] = Table[1][i];
		}
	}
	else if (SR == 96000.000)
	{
		for (int i = 0; i<512; i++)
		{
			oldj = i * 2;
			for (int j = 0; j<2; j++) {
				RusterArrayX[j + oldj] = Table[0][i];
				RusterArrayX2[j + oldj] = Table[0][i];
				RusterArrayX3[i + oldj] = Table[0][i];
			}
		}
		for (int i = 0; i<512; i++)
		{
			oldj = i * 2;
			for (int j = 0; j<2; j++) {
				RusterArrayY[j + oldj] = Table[1][i];
				RusterArrayY2[j + oldj] = Table[1][i];
				RusterArrayY3[j + oldj] = Table[1][i];
			}
		}
	}
	else if (SR == 192000.000)
	{
		for (int i = 0; i<512; i++)
		{
			oldj = i * 4;
			for (int j = 0; j<4; j++) {
				RusterArrayX[j + oldj] = Table[0][i];
				RusterArrayX2[j + oldj] = Table[0][i];
				RusterArrayX3[j + oldj] = Table[0][i];
			}
		}
		for (int i = 0; i<512; i++)
		{
			oldj = i * 4;
			for (int j = 0; j<4; j++) {
				RusterArrayY[j + oldj] = Table[1][i];
				RusterArrayY2[j + oldj] = Table[1][i];
				RusterArrayY3[j + oldj] = Table[1][i];
			}
		}
	}
	PlayTones = false;
}
#pragma pack(pop)

static bool initialized; unsigned int lol;

extern "C" {
	__declspec (dllexport) int InitAriadni(TAriadniCallBackFunc i);
	__declspec (dllexport) double MoveBeam(double x, double y);
	__declspec (dllexport) int SetScanOutline(double Array[2][512]);
	__declspec (dllexport) double GetSpinningFreq();
	__declspec (dllexport) int ShowGui(bool State);
	__declspec (dllexport) double SetSpinningFreq(double frequency);
	__declspec (dllexport) int ShowScanOutline(bool States);
	__declspec (dllexport) void AbortLineScan();
	__declspec (dllexport) int GetAriadniStatus();
	__declspec (dllexport) int LockGui(BOOL State);
	__declspec (dllexport) int ReleaseAriadni();
	__declspec (dllexport) int SetLineScanData(TDestChannels Channels, int EventCount, TScanData * Data);
	__declspec (dllexport)  int GetAriadniStatus();
}

class MainContentComponent : public AudioAppComponent,
	public SliderListener,
	public TextEditorListener,
	public ComboBoxListener,
	public ButtonListener
{
	double FoutSample[9][8024];
	int counterStep;
public:
	MainContentComponent() :Tamp(0), TestingVal(0), VlaDelay(0.0), counterStep(0), Amp1(0.0), Amp2(0.0), nev(0.0), Amp11(0.0), Amp12(0.0), Amp22(0.0), Amp21(0.0)
	{
		memset(FoutSample, 0, sizeof(FoutSample[0][0]) * 8 * 8024);
		SR = 0.0;
		waiting = false;
		setSize(361, 447);
		ScanDuration = 0;
		Temp = 0;
		if (SUCCEEDED(SHGetFolderPath(NULL,
			CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE,
			NULL,
			0,
			szPath)))
		{
			for (int i = 0; i < MAX_PATH; i++)
				szPath2[i] = szPath[i];

			PathAppend(szPath, TEXT("NanoMEGAS"));
			String directoryName;
			directoryName << szPath;
			CreateDirectory(directoryName.toUTF8(), NULL);
			directoryName = directoryName + "\\AcomPatch";
			CreateDirectory(directoryName.toUTF8(), NULL);
			PathAppend(szPath, TEXT("\\AcomPatch\\FileProperties.xml"));
			HANDLE hFile = CreateFile(szPath, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL); CloseHandle(hFile);
			PathAppend(szPath2, TEXT("NanoMEGAS\\AcomPatch\\HardwareSettings.xml"));
			HANDLE hFile2 = CreateFile(szPath2, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL); CloseHandle(hFile2);
		}

		addAndMakeVisible(groupComponent2 = new GroupComponent("new group",
			TRANS("Beam Scan Pivot Point")));
		groupComponent2->setTextLabelPosition(Justification::centred);

		addAndMakeVisible(groupComponent = new GroupComponent("new group",
			TRANS("Beam Scan")));
		groupComponent->setTextLabelPosition(Justification::centred);

		addAndMakeVisible(FileSelection = new ComboBox("new combo box"));
		FileSelection->setEditableText(false);
		FileSelection->setJustificationType(Justification::centredLeft);
		FileSelection->setTextWhenNoChoicesAvailable(TRANS("(no choices)"));
		FileSelection->addListener(this);
		FileSelection->setSelectedId(1 + true);
		FileSelection->setBounds(191, 102, 110, 23);

		addAndMakeVisible(textButton = new TextButton("new button"));
		textButton->setButtonText(TRANS("Scan Settings"));
		textButton->setConnectedEdges(Button::ConnectedOnRight | Button::ConnectedOnBottom);
		textButton->addListener(this);
		textButton->setColour(TextButton::ColourIds::buttonColourId, Colours::burlywood);
		addAndMakeVisible(textButton2 = new TextButton("new button"));
		textButton2->setButtonText(TRANS("General Settings"));
		textButton2->setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnBottom);
		textButton2->addListener(this);

		addAndMakeVisible(textEditor = new TextEditor("new text editor"));
		textEditor->setMultiLine(false);
		textEditor->setReturnKeyStartsNewLine(false);
		textEditor->setReadOnly(false);
		textEditor->setScrollbarsShown(true);
		textEditor->setCaretVisible(true);
		textEditor->setPopupMenuEnabled(true);
		textEditor->addListener(this);
		textEditor->setColour(TextEditor::outlineColourId, Colours::lightgrey);
		textEditor->setText(String());

		addAndMakeVisible(textEditor2 = new TextEditor("new text editor"));
		textEditor2->setMultiLine(false);
		textEditor2->setReturnKeyStartsNewLine(false);
		textEditor2->setReadOnly(false);
		textEditor2->setScrollbarsShown(true);
		textEditor2->setCaretVisible(true);
		textEditor2->setPopupMenuEnabled(true);
		textEditor2->setText(String());
		textEditor2->addListener(this);
		textEditor2->setColour(TextEditor::outlineColourId, Colours::lightgrey);
		textEditor->setBounds(233, 158, 69, 24);
		textEditor2->setBounds(233, 232 - 10, 69, 24);

		addAndMakeVisible(labelf = new Label("new label",
			TRANS("Fine Increments")));
		labelf->setFont(Font(14.00f, Font::bold));
		labelf->setJustificationType(Justification::centredLeft);
		labelf->setEditable(false, false, false);
		labelf->setColour(TextEditor::textColourId, Colours::black);
		labelf->setColour(TextEditor::backgroundColourId, Colour(0x00000000));
		addAndMakeVisible(labelfa = new Label("new label",
			TRANS("Fine steps for Amplitude:")));
		labelfa->setFont(Font(14.70f, Font::plain));
		labelfa->setJustificationType(Justification::centredLeft);
		labelfa->setEditable(false, false, false);
		labelfa->setColour(TextEditor::textColourId, Colours::black);
		labelfa->setColour(TextEditor::backgroundColourId, Colour(0x00000000));
		addAndMakeVisible(labelfp = new Label("new label",
			TRANS("Fine steps for Rotation:")));
		labelfp->setFont(Font(14.70f, Font::plain));
		labelfp->setJustificationType(Justification::centredLeft);
		labelfp->setEditable(false, false, false);
		labelfp->setColour(TextEditor::textColourId, Colours::black);
		labelfp->setColour(TextEditor::backgroundColourId, Colour(0x00000000));
		labelf->setBounds(52, 125, 100, 24);
		labelfa->setBounds(52, 159, 180, 26);
		labelfp->setBounds(52, 235, 180, 26);

		addAndMakeVisible(FineButton = new ToggleButton("Fine  Button"));
		FineButton->setButtonText(TRANS("Fine"));
		FineButton->addListener(this);
		FineButton->setColour(ToggleButton::textColourId, Colours::black);
		FineButton->setBounds(24, 48, 55, 24);

		addAndMakeVisible(SaveButton = new TextButton("Alingment  Button"));
		SaveButton->setButtonText(TRANS("Save Settings"));
		SaveButton->addListener(this);
		SaveButton->setConnectedEdges(Button::ConnectedOnTop | Button::ConnectedOnBottom);
		SaveButton->setColour(ToggleButton::textColourId, Colours::black);
		SaveButton->setBounds(getWidth() / 2 - 48, getHeight() - 33, 98, 23);

		addAndMakeVisible(SettingButton = new TextButton("Setting's button"));
		SettingButton->setButtonText(TRANS("Hardware Config"));
		SettingButton->addListener(this);
		SettingButton->setConnectedEdges(Button::ConnectedOnTop | Button::ConnectedOnBottom);
		SettingButton->setColour(ToggleButton::textColourId, Colours::black);
		SettingButton->setBounds(123, 312, 112, 24);

		addAndMakeVisible(BButton = new TextButton("Setting's button"));
		BButton->setButtonText(TRANS("Back up the Xml files"));
		BButton->addListener(this);
		BButton->setConnectedEdges(Button::ConnectedOnTop | Button::ConnectedOnBottom);
		BButton->setColour(ToggleButton::textColourId, Colours::black);
		BButton->setBounds(123, 258, 112, 24);
		BButton->setVisible(false);

		PlayTones = true;

		addAndMakeVisible(Rusterslider1 = new Slider("new slider"));
		Rusterslider1->setRange(0.0, 1.0);
		Rusterslider1->setSliderStyle(Slider::LinearHorizontal);
		Rusterslider1->setTextBoxStyle(Slider::TextBoxAbove, false, 50, 20);
		Rusterslider1->setColour(Slider::backgroundColourId, Colour(0x000f0909));
		Rusterslider1->setColour(Slider::thumbColourId, Colour(0xff84808d));
		Rusterslider1->setColour(Slider::trackColourId, Colour(0x7f322c2c));
		Rusterslider1->addListener(this);
		Rusterslider1->sliderType("AmplitudeSlider");

		addAndMakeVisible(Delay = new Slider("new slider"));
		Delay->setRange(0.0, 1.0);
		Delay->setSliderStyle(Slider::LinearHorizontal);
		Delay->setTextBoxStyle(Slider::TextBoxAbove, false, 50, 20);
		Delay->setColour(Slider::backgroundColourId, Colour(0x000f0909));
		Delay->setColour(Slider::thumbColourId, Colour(0xff84808d));
		Delay->setColour(Slider::trackColourId, Colour(0x7f322c2c));
		Delay->addListener(this);
		Delay->sliderType("AmplitudeSlider");
		Delay->setVisible(false);

		addAndMakeVisible(Rusterslider2 = new Slider("new slider"));
		Rusterslider2->setRange(0.0, 1.0);
		Rusterslider2->setSliderStyle(Slider::LinearHorizontal);
		Rusterslider2->setTextBoxStyle(Slider::TextBoxAbove, false, 50, 20);
		Rusterslider2->setColour(Slider::backgroundColourId, Colour(0x000f0909));
		Rusterslider2->setColour(Slider::thumbColourId, Colour(0xff84808d));
		Rusterslider2->setColour(Slider::trackColourId, Colour(0x7f322c2c));
		Rusterslider2->addListener(this);
		Rusterslider2->sliderType("AmplitudeSlider");

		addAndMakeVisible(label = new Label("new label",
			TRANS("BDUX (CH1 Ampl.)")));
		label->setFont(Font(15.00f, Font::plain));
		label->setJustificationType(Justification::centredLeft);
		label->setEditable(false, false, false);
		label->setColour(TextEditor::textColourId, Colours::black);
		label->setColour(TextEditor::backgroundColourId, Colour(0x00000000));

		addAndMakeVisible(label2 = new Label("new label",
			TRANS("BDUY (CH2 Ampl.)")));
		label2->setFont(Font(15.00f, Font::plain));
		label2->setJustificationType(Justification::centredLeft);
		label2->setEditable(false, false, false);
		label2->setColour(TextEditor::textColourId, Colours::black);
		label2->setColour(TextEditor::backgroundColourId, Colour(0x00000000));

		addAndMakeVisible(RustersliderRot = new Slider("new slider"));
		RustersliderRot->setRange(0.0, 6.28);
		RustersliderRot->setSliderStyle(Slider::LinearHorizontal);
		RustersliderRot->setTextBoxStyle(Slider::TextBoxAbove, false, 50, 20);
		RustersliderRot->setColour(Slider::backgroundColourId, Colour(0x000f0909));
		RustersliderRot->setColour(Slider::thumbColourId, Colour(0xff84808d));
		RustersliderRot->setColour(Slider::trackColourId, Colour(0x7f322c2c));
		RustersliderRot->addListener(this);
		RustersliderRot->sliderType("PhaseSlider");

		addAndMakeVisible(label3 = new Label("new label",
			TRANS("Ruster Rotation")));
		label3->setFont(Font(15.00f, Font::plain));
		label3->setJustificationType(Justification::centredLeft);
		label3->setEditable(false, false, false);
		label3->setColour(TextEditor::textColourId, Colours::black);
		label3->setColour(TextEditor::backgroundColourId, Colour(0x00000000));

		addAndMakeVisible(label4 = new Label("new label",
			TRANS("Mode:")));
		label4->setFont(Font(15.00f, Font::bold));
		label4->setJustificationType(Justification::centredLeft);
		label4->setEditable(false, false, false);
		label4->setColour(TextEditor::textColourId, Colours::black);
		label4->setColour(TextEditor::backgroundColourId, Colour(0x00000000));

		addAndMakeVisible(Rusterslider3 = new Slider("new slider"));
		Rusterslider3->setRange(-2.0, 2.0);
		Rusterslider3->setSliderStyle(Slider::LinearHorizontal);
		Rusterslider3->setTextBoxStyle(Slider::TextBoxAbove, false, 50, 20);
		Rusterslider3->setColour(Slider::backgroundColourId, Colour(0x000f0909));
		Rusterslider3->setColour(Slider::thumbColourId, Colour(0xff84808d));
		Rusterslider3->setColour(Slider::trackColourId, Colour(0x7f322c2c));
		Rusterslider3->addListener(this);
		Rusterslider3->sliderType("AmplitudeSlider");

		addAndMakeVisible(Rusterslider4 = new Slider("new slider"));
		Rusterslider4->setRange(-0.2, 0.2);
		Rusterslider4->setSliderStyle(Slider::LinearHorizontal);
		Rusterslider4->setTextBoxStyle(Slider::TextBoxAbove, false, 50, 20);
		Rusterslider4->setColour(Slider::backgroundColourId, Colour(0x000f0909));
		Rusterslider4->setColour(Slider::thumbColourId, Colour(0xff84808d));
		Rusterslider4->setColour(Slider::trackColourId, Colour(0x7f322c2c));
		Rusterslider4->sliderType("AmplitudeSlider");
		Rusterslider4->addListener(this);

		addAndMakeVisible(label5 = new Label("new label",
			TRANS("BDLXX")));
		label5->setFont(Font(15.00f, Font::plain));
		label5->setJustificationType(Justification::centredLeft);
		label5->setEditable(false, false, false);
		label5->setColour(TextEditor::textColourId, Colours::black);
		label5->setColour(TextEditor::backgroundColourId, Colour(0x00000000));

		addAndMakeVisible(label6 = new Label("new label",
			TRANS("BDLXY")));
		label6->setFont(Font(15.00f, Font::plain));
		label6->setJustificationType(Justification::centredLeft);
		label6->setEditable(false, false, false);
		label6->setColour(TextEditor::textColourId, Colours::black);
		label6->setColour(TextEditor::backgroundColourId, Colour(0x00000000));

		addAndMakeVisible(mode = new Label("new label",
			TRANS("Operating file:")));
		mode->setFont(Font(15.00f, Font::plain));
		mode->setJustificationType(Justification::centredLeft);
		mode->setEditable(false, false, false);
		mode->setColour(TextEditor::textColourId, Colours::black);
		mode->setColour(TextEditor::backgroundColourId, Colour(0x00000000));
		mode->setBounds(52, 102, 100, 24);

		addAndMakeVisible(Rusterslider5 = new Slider("new slider"));
		Rusterslider5->setRange(-0.2, 0.2);
		Rusterslider5->setSliderStyle(Slider::LinearHorizontal);
		Rusterslider5->setTextBoxStyle(Slider::TextBoxAbove, false, 50, 20);
		Rusterslider5->setColour(Slider::backgroundColourId, Colour(0x000f0909));
		Rusterslider5->setColour(Slider::thumbColourId, Colour(0xff84808d));
		Rusterslider5->setColour(Slider::trackColourId, Colour(0x7f322c2c));
		Rusterslider5->sliderType("AmplitudeSlider");
		Rusterslider5->addListener(this);

		addAndMakeVisible(Rusterslider6 = new Slider("new slider"));
		Rusterslider6->setRange(-2.0, 2.0);
		Rusterslider6->setSliderStyle(Slider::LinearHorizontal);
		Rusterslider6->setTextBoxStyle(Slider::TextBoxAbove, false, 50, 20);
		Rusterslider6->setColour(Slider::backgroundColourId, Colour(0x000f0909));
		Rusterslider6->setColour(Slider::thumbColourId, Colour(0xff84808d));
		Rusterslider6->setColour(Slider::trackColourId, Colour(0x7f322c2c));
		Rusterslider6->addListener(this);
		Rusterslider6->sliderType("AmplitudeSlider");

		addAndMakeVisible(label7 = new Label("new label",
			TRANS("BDLYX")));
		label7->setFont(Font(15.00f, Font::plain));
		label7->setJustificationType(Justification::centredLeft);
		label7->setEditable(false, false, false);
		label7->setColour(TextEditor::textColourId, Colours::black);
		label7->setColour(TextEditor::backgroundColourId, Colour(0x00000000));

		addAndMakeVisible(DelayLabel = new Label("new label",
			TRANS("Delay:")));
		DelayLabel->setFont(Font(13.00f, Font::plain));
		DelayLabel->setJustificationType(Justification::centredLeft);
		DelayLabel->setEditable(false, false, false);
		DelayLabel->setColour(TextEditor::textColourId, Colours::black);
		DelayLabel->setColour(TextEditor::backgroundColourId, Colour(0x00000000));
		DelayLabel->setVisible(false);

		addAndMakeVisible(labelv = new Label("new label",
			TRANS("version: 3.0.0")));
		labelv->setFont(Font(13.00f, Font::plain));
		labelv->setJustificationType(Justification::centred);
		labelv->setEditable(false, false, false);
		labelv->setColour(TextEditor::textColourId, Colours::black);
		labelv->setColour(TextEditor::backgroundColourId, Colour(0x00000000));
		labelv->setBounds(123, 377, 112, 23);

		addAndMakeVisible(label8 = new Label("new label",
			TRANS("BDLYY")));
		label8->setFont(Font(15.00f, Font::plain));
		label8->setJustificationType(Justification::centredLeft);
		label8->setEditable(false, false, false);
		label8->setColour(TextEditor::textColourId, Colours::black);
		label8->setColour(TextEditor::backgroundColourId, Colour(0x00000000));
		//Size and positioning of each component
		groupComponent2->setBounds(24, 237, 312, 160);
		groupComponent->setBounds(24, 64, 312, 160);
		Rusterslider1->setBounds(44, 106, 126, 44);
		Rusterslider2->setBounds(190, 106, 126, 44);
		RustersliderRot->setBounds(proportionOfWidth(0.4986f) - (126 / 2), 171, 126, 44);
		Rusterslider3->setBounds(44, 279, 126, 44);
		Rusterslider4->setBounds(190, 279, 126, 44);
		Rusterslider5->setBounds(44, 344, 126, 44);
		Rusterslider6->setBounds(190, 344, 126, 44);
		Delay->setBounds(120, 205, 120, 44);
		Delay->setAlwaysOnTop(true);
		DelayLabel->setBounds(60, 184, 100, 66);
		DelayLabel->setAlwaysOnTop(false);
		label->setBounds(44, 82, 126, 22);
		label2->setBounds(190, 82, 126, 22);
		label3->setBounds(125, 144, 115, 22);
		label4->setBounds(getWidth() / 2 - 50, 3, 100, 22);
		label5->setBounds(83, 257, 50, 22);
		label6->setBounds(230, 257, 50, 22);
		label7->setBounds(83, 321, 50, 22);
		label8->setBounds(230, 321, 50, 22);
		labelv->setVisible(false);
		textButton2->setBounds(180, 25, 100, 24);
		textButton->setBounds(80, 25, 100, 24);
		//Paths for Grid lines
		internalPath1.clear();
		internalPath1.startNewSubPath(16.0f, 48.0f);
		internalPath1.lineTo(16.0f, 404.0f);
		internalPath1.closeSubPath();

		internalPath2.clear();
		internalPath2.startNewSubPath(344.0f, 48.0f);
		internalPath2.lineTo(344.0f, 404.0f);
		internalPath2.closeSubPath();

		internalPath3.clear();
		internalPath3.startNewSubPath(16.0f, 48.0f);
		internalPath3.lineTo(80.0f, 48.0f);
		internalPath3.closeSubPath();

		internalPath4.clear();
		internalPath4.startNewSubPath(280.0f, 48.0f);
		internalPath4.lineTo(344.0f, 48.0f);
		internalPath4.closeSubPath();

		internalPath5.clear();
		internalPath5.startNewSubPath(16.0f, 404.0f);
		internalPath5.lineTo(344.0f, 404.0f);
		internalPath5.closeSubPath();

		//Set sliders to normal mode not to fine
		Rusterslider1->sliderMode("Normal");
		Rusterslider2->sliderMode("Normal");
		Rusterslider3->sliderMode("Normal");
		Rusterslider6->sliderMode("Normal");
		RustersliderRot->sliderMode("Normal");

		mode->setVisible(false); labelf->setVisible(false);
		labelfa->setVisible(false); labelfp->setVisible(false);
		FileSelection->setVisible(false); SettingButton->setVisible(false);
		textEditor->setVisible(false); textEditor2->setVisible(false);
		bool bLoadResult2 = mDoc.Load(szPath2);
		if (bLoadResult2 == false)
			InitializeHardwareXML();
		InitializeAudioDevice();
		bool bLoadResult = mDoc.Load(szPath);
		if (bLoadResult == false)
			InitializeXML();
		String sAttrib2;
		mDoc.ResetPos();
		mDoc.FindElem("CompleteAlignmentDictionary");
		mDoc.IntoElem();

		for (int i = 0; i < 5; i++)
		{
			mDoc.FindElem("Alignment");
			mDoc.IntoElem();
			mDoc.FindElem("Name");
			sAttrib2 = "";
			sAttrib2 << mDoc.GetElemContent();
			FileSelection->addItem(sAttrib2, i + 1);
			mDoc.OutOfElem();
		}
		StartUpFile();
	}

	~MainContentComponent()
	{
		audioWindow();
		if (VisibilityState == true) {
			int StateOfWindow = AlertWindow::showNativeDialogBox("Exit from Acom Scanning Patch!", ("Do you want to save the changes in your Xml file?"), true);
			if (StateOfWindow == 1)
				SaveXml();
		}
		Rusterslider1 = nullptr;
		Rusterslider2 = nullptr;
		label = nullptr;
		label2 = nullptr;
		RustersliderRot = nullptr;
		label3 = nullptr;
		label4 = nullptr;
		Rusterslider3 = nullptr;
		Rusterslider4 = nullptr;
		label5 = nullptr;
		label6 = nullptr;
		Rusterslider5 = nullptr;
		Rusterslider6 = nullptr;
		label7 = nullptr;
		label8 = nullptr;
		shutdownAudio();
	}

	void paint(Graphics& g)
	{
		g.fillAll(Colour(0xffebebe6));
		g.setColour(Colours::cadetblue);
		g.strokePath(internalPath1, PathStrokeType(2.000f));

		g.setColour(Colours::cadetblue);
		g.strokePath(internalPath2, PathStrokeType(2.000f));

		g.setColour(Colours::cadetblue);
		g.strokePath(internalPath3, PathStrokeType(2.000f));

		g.setColour(Colours::cadetblue);
		g.strokePath(internalPath4, PathStrokeType(2.000f));

		g.setColour(Colours::cadetblue);
		g.strokePath(internalPath5, PathStrokeType(2.000f));

	}

	void resized() override { }

	void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override
	{
		String message;
		message << "Preparing to play audio...\n";
		message << " samplesPerBlockExpected = " << samplesPerBlockExpected << "\n";
		message << " sampleRate = " << sampleRate;
		Logger::getCurrentLogger()->writeToLog(message);
	}

	void releaseResources() override
	{
		Logger::getCurrentLogger()->writeToLog("Releasing audio resources");
	}

	void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) override
	{
		int Events = count;
		int CoUnt = 0; int Line = 0; l = 0;
		int raise = SR / 100;
		SR = sampleRate;
		// Get a pointer to the start sample in the buffer for this audio output channel
		CH1Buffer = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
		CH2Buffer = bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample);
		CH3Buffer = bufferToFill.buffer->getWritePointer(2, bufferToFill.startSample);
		CH4Buffer = bufferToFill.buffer->getWritePointer(3, bufferToFill.startSample);
		CH5Buffer = bufferToFill.buffer->getWritePointer(4, bufferToFill.startSample);
		CH6Buffer = bufferToFill.buffer->getWritePointer(5, bufferToFill.startSample);
		CH7Buffer = bufferToFill.buffer->getWritePointer(6, bufferToFill.startSample);
		CH8Buffer = bufferToFill.buffer->getWritePointer(7, bufferToFill.startSample);
		CameraBuffer = bufferToFill.buffer->getWritePointer(9, bufferToFill.startSample);
		CameraBuffer2 = bufferToFill.buffer->getWritePointer(8, bufferToFill.startSample);
		if (waiting == false && waiting2 == false)//No scan data the waiting = false
		{
			if (PlayTones == true) {//Play Tones means be ready to recieve offsets
				for (int sample = 0; sample < bufferToFill.numSamples; ++sample)
				{
					FoutSample[0][sample] = AstarOffset1;
					FoutSample[1][sample] = AstarOffset2;
					CH1Buffer[sample] = FoutSample[0][sample];
					CH2Buffer[sample] = FoutSample[1][sample];
				}
			}
			else if (PlayTones == false)//Play the square
			{
				for (int sample = 0; sample < bufferToFill.numSamples; ++sample)
				{
					//counterStep == raise
					if (counterStep == 512)//If the Square dta finished go from the begin.
						counterStep = 0;
					FoutSample[0][sample] = (RusterArrayX[counterStep] * std::cos(nev) + RusterArrayY[counterStep] * std::sin(nev))*Amp1;
					FoutSample[1][sample] = (RusterArrayY[counterStep] * std::cos(nev) + RusterArrayX[counterStep] * -std::sin(nev))*Amp2;
					FoutSample[2][sample] = (RusterArrayX[counterStep] * (std::cos(nev) + Amp12) + RusterArrayY[counterStep] * (std::sin(nev) + Amp12))*Amp1;;
					FoutSample[3][sample] = (RusterArrayY[counterStep] * (std::cos(nev) + (Amp21*-1)) + RusterArrayX[(int)counterStep] * -(std::sin(nev) + (Amp21*-1)))*Amp2;
					FoutSample[2][sample] = FoutSample[2][sample] * (Amp11);
					FoutSample[3][sample] = FoutSample[3][sample] * (Amp22);
					counterStep++;
					if (EnableSCan == 1)//EnableSCan = 1 when ShowOutlineScan(true) is executed so the square's data goes to Gis output
					{
						CH1Buffer[sample] = FoutSample[0][sample];
						CH2Buffer[sample] = FoutSample[1][sample];
						CH3Buffer[sample] = FoutSample[2][sample];
						CH4Buffer[sample] = FoutSample[3][sample];
					}
				}
			}
		}
		//Waiting = true while scan is still on process.
		else if (waiting == true || (waiting = false && waiting2 ==true))
		{
			for (int i = 0; i < bufferToFill.numSamples; i++)
			{
				if (ScanDuration < DurationOfLine) {
					if (Keeper < TimeScanPerEvent[KeeperArray]) {
						CH1Buffer[i] = (Ch2DataScan[KeeperArray] * std::cos(nev) + Ch1DataScan[KeeperArray] * std::sin(nev))*Amp1;
						CH2Buffer[i] = (Ch1DataScan[KeeperArray] * std::cos(nev) + Ch2DataScan[KeeperArray] * -std::sin(nev))*Amp2;
						CH3Buffer[i] = ((Ch2DataScan[KeeperArray] * (std::cos(nev) + Amp12) + Ch1DataScan[KeeperArray] * (std::sin(nev) + Amp12))*Amp1)*Amp11;
						CH4Buffer[i] = ((Ch1DataScan[KeeperArray] * (std::cos(nev) + (Amp21*-1)) + Ch2DataScan[KeeperArray] * -(std::sin(nev) + (Amp21*-1)))*Amp2)*Amp22;
						CH5Buffer[i] = (float)Ch6DataScan[KeeperArray] * Amp1;
						CH6Buffer[i] = (float)Ch5DataScan[KeeperArray] * Amp2;

						//if ((Keeper - (int)VlaDelay) >= 0) {
						CameraBuffer[i] = (float)ChCameraDataScan[Keeper];// (Keeper - (int)VlaDelay)];
						CameraBuffer2[i] = (float)ChCameraDataScan9[Keeper];// (Keeper - (int)VlaDelay)];
						/*}
						else {
							CameraBuffer[i] = 0.0;
							CameraBuffer2[i] = 0.0;
						}*/
						Keeper++;
					}
					else {
						CH1Buffer[i] = (Ch2DataScan[KeeperArray] * std::cos(nev) + Ch1DataScan[KeeperArray] * std::sin(nev))*Amp1;
						CH2Buffer[i] = (Ch1DataScan[KeeperArray] * std::cos(nev) + Ch2DataScan[KeeperArray] * -std::sin(nev))*Amp2;
						CH3Buffer[i] = ((Ch2DataScan[KeeperArray] * (std::cos(nev) + Amp12) + Ch1DataScan[KeeperArray] * (std::sin(nev) + Amp12))*Amp1)*Amp11;
						CH4Buffer[i] = ((Ch1DataScan[KeeperArray] * (std::cos(nev) + (Amp21*-1)) + Ch2DataScan[KeeperArray] * -(std::sin(nev) + (Amp21*-1)))*Amp2)*Amp22;
						CH5Buffer[i] = (float)Ch6DataScan[KeeperArray] * Amp1;
						CH6Buffer[i] = (float)Ch5DataScan[KeeperArray] * Amp2;
						//If the delay slider !=0 go in
						//if ((Keeper - (int)VlaDelay) >= 0) {
							CameraBuffer[i] = (float)ChCameraDataScan[Keeper];// (Keeper - (int)VlaDelay)];
							CameraBuffer2[i] = (float)ChCameraDataScan9[Keeper];// (Keeper - (int)VlaDelay)];
						/*}
						else {
							CameraBuffer[i] = 0.0;
							CameraBuffer2[i] = 0.0;
						}*/
						KeeperArray = KeeperArray + 1;
					}
					//Raise the Scanduration for 1 sample each time
					ScanDuration++;
				}
				else if (ScanDuration == DurationOfLine)
				{
					//waiting = false;
					if (TestingVal < (int)VlaDelay) {
						if (Ch1DataScan != NULL) {
							CH1Buffer[i] = (Ch2DataScan[KeeperArray] * std::cos(nev) + Ch1DataScan[KeeperArray] * std::sin(nev))*Amp1;
							CH2Buffer[i] = (Ch1DataScan[KeeperArray] * std::cos(nev) + Ch2DataScan[KeeperArray] * -std::sin(nev))*Amp2;
							CH3Buffer[i] = ((Ch2DataScan[KeeperArray] * (std::cos(nev) + Amp12) + Ch1DataScan[KeeperArray] * (std::sin(nev) + Amp12))*Amp1)*Amp11;
							CH4Buffer[i] = ((Ch1DataScan[KeeperArray] * (std::cos(nev) + (Amp21*-1)) + Ch2DataScan[KeeperArray] * -(std::sin(nev) + (Amp21*-1)))*Amp2)*Amp22;
							CH5Buffer[i] = (float)Ch6DataScan[KeeperArray] * Amp1;
							CH6Buffer[i] = (float)Ch5DataScan[KeeperArray] * Amp2;
							//If the delay slider !=0 go in
							//if ((Keeper - (int)VlaDelay) >= 0) {
								CameraBuffer[i] = (float)ChCameraDataScan[Keeper];// (Keeper - (int)VlaDelay)];
								CameraBuffer2[i] = (float)ChCameraDataScan9[Keeper];// (Keeper - (int)VlaDelay)];
							/*}
							else {
								CameraBuffer[i] = 0.0;
								CameraBuffer2[i] = 0.0;
							}*/
						}
						TestingVal++;
						waiting = false; waiting2 = true;
						//CallBack(100, (double)ScanDuration);
					}
					else if (TestingVal == (int)VlaDelay || VlaDelay == 0.00000) {
						ArStatus = 1;
						if (Ch1DataScan != NULL) {
							CH1Buffer[i] = (Ch2DataScan[KeeperArray] * std::cos(nev) + Ch1DataScan[KeeperArray] * std::sin(nev))*Amp1;
							CH2Buffer[i] = (Ch1DataScan[KeeperArray] * std::cos(nev) + Ch2DataScan[KeeperArray] * -std::sin(nev))*Amp2;
							CH3Buffer[i] = ((Ch2DataScan[KeeperArray] * (std::cos(nev) + Amp12) + Ch1DataScan[KeeperArray] * (std::sin(nev) + Amp12))*Amp1)*Amp11;
							CH4Buffer[i] = ((Ch1DataScan[KeeperArray] * (std::cos(nev) + (Amp21*-1)) + Ch2DataScan[KeeperArray] * -(std::sin(nev) + (Amp21*-1)))*Amp2)*Amp22;
							CH5Buffer[i] = (float)Ch6DataScan[KeeperArray] * Amp1;
							CH6Buffer[i] = (float)Ch5DataScan[KeeperArray] * Amp2;
							//If the delay slider !=0 go in
							/*if ((Keeper - (int)VlaDelay) >= 0) {*/
								CameraBuffer[i] = (float)ChCameraDataScan[Keeper];// (Keeper - (int)VlaDelay)];
								CameraBuffer2[i] = (float)ChCameraDataScan9[Keeper];// (Keeper - (int)VlaDelay)];
							/*}
							else {
								CameraBuffer[i] = 0.0;
								CameraBuffer2[i] = 0.0;
							}*/
						}
						if (waiting != false) {//When the line scan ends delte th arrays and send notification back
							delete[] Ch1DataScan; delete[] Ch2DataScan; delete[] Ch3DataScan; Ch1DataScan = NULL; Ch2DataScan = NULL; Ch3DataScan = NULL;
							delete[] Ch4DataScan; delete[] Ch5DataScan; delete[] Ch6DataScan; delete[] ChCameraDataScan; delete[] ChCameraDataScan9;
							Ch4DataScan = NULL; Ch5DataScan = NULL; Ch6DataScan = NULL; ChCameraDataScan = NULL; ChCameraDataScan9 = NULL;
							CallBack(ArStatus, (double)ScanDuration);//pp((double)CH1Buffer[i], (double) CH2Buffer[i]);
							ScanDuration = DurationOfLine;
							waiting = false; waiting2 = false; TestingVal = 0;
						}
					}

				}
			}
		}
	}

	void textEditorTextChanged(TextEditor& textEditorThatHasChanged)
	{
		if (FineButton->getToggleState() == true)
		{
			FineButton->setToggleState(false, true);
			FineButton->setToggleState(true, true);
			FineButton->setToggleState(true, false);
		}
		else if (FineButton->getToggleState() == false)
		{
			FineButton->setToggleState(true, true);
			FineButton->setToggleState(false, true);
			FineButton->setToggleState(false, false);
		}
		double number = textEditor->getText().getDoubleValue();
		if (number > 0.0001)
		{
			AlertWindow::showMessageBox(AlertWindow::InfoIcon, "Wrong value!", ("The fine steps for amplitude must be 0.0001 or below!"), "Ok", nullptr);
			textEditor->setText("0.0001", true);
		}
	}

	void buttonClicked(Button* buttonThatWasClicked)
	{
		if (buttonThatWasClicked == SettingButton)
		{
			double ArrEmpty[2][512];
			for (int j = 0; j<2; j++)
				for (int i = 0; i<512; i++)
					ArrEmpty[j][i] = 0.0000;
			setRuster(ArrEmpty);
			PlayTones = NULL;
			SettingsForAudio();
		}
		else if (buttonThatWasClicked == BButton)
		{
			juce::FileChooser myChooser("Please select the location for the backup of the file...",
				File::getSpecialLocation(File::userHomeDirectory),
				"*.XML");
			mDoc.Load("C:\\Users\\User\\AppData\\Roaming\\AcomPatch\\FilePropertier.xml");
			CString copyFile = mDoc.GetDoc();
			mDoc.SetDoc(copyFile);
			if (myChooser.browseForFileToSave(true))
			{
				File f(myChooser.getResult());
				mDoc.WriteTextFile(f.getFullPathName().toUTF8(), copyFile, 0, 0);
			}
		}
		else if (buttonThatWasClicked == textButton)
		{
			textButton->setColour(TextButton::ColourIds::buttonColourId, Colours::burlywood);
			textButton2->setColour(TextButton::ColourIds::buttonColourId, Colour(0xFFBBBBFF));
			groupComponent2->setVisible(true);
			groupComponent->setVisible(true);
			Rusterslider1->setVisible(true);
			Rusterslider2->setVisible(true);
			RustersliderRot->setVisible(true);
			Rusterslider3->setVisible(true);
			Rusterslider4->setVisible(true);
			Rusterslider5->setVisible(true);
			Rusterslider6->setVisible(true);
			Delay->setVisible(false);
			DelayLabel->setVisible(false);
			label->setVisible(true);
			label2->setVisible(true);
			label3->setVisible(true);
			label5->setVisible(true);
			label6->setVisible(true);
			label7->setVisible(true);
			label8->setVisible(true);
			labelf->setVisible(false);
			labelfa->setVisible(false);
			labelfp->setVisible(false);
			FileSelection->setVisible(false);
			textEditor->setVisible(false);
			textEditor2->setVisible(false);
			SettingButton->setVisible(false);
			FineButton->setVisible(true);
			mode->setVisible(false);
			BButton->setVisible(false);
			labelv->setVisible(false);
		}
		else if (buttonThatWasClicked == textButton2)
		{
			textButton2->setColour(TextButton::ColourIds::buttonColourId, Colours::burlywood);
			textButton->setColour(TextButton::ColourIds::buttonColourId, Colour(0xFFBBBBFF));
			groupComponent2->setVisible(false);
			groupComponent->setVisible(false);
			Rusterslider1->setVisible(false);
			Rusterslider2->setVisible(false);
			RustersliderRot->setVisible(false);
			Rusterslider3->setVisible(false);
			Rusterslider4->setVisible(false);
			Rusterslider5->setVisible(false);
			Rusterslider6->setVisible(false);
			Delay->setVisible(true);
			DelayLabel->setVisible(true);
			label->setVisible(false);
			label2->setVisible(false);
			BButton->setVisible(true);
			label3->setVisible(false);
			label5->setVisible(false);
			label6->setVisible(false);
			label7->setVisible(false);
			label8->setVisible(false);
			labelf->setVisible(false);
			labelfa->setVisible(true);
			labelfp->setVisible(false);
			FileSelection->setVisible(true);
			textEditor->setVisible(true);
			textEditor2->setVisible(false);
			SettingButton->setVisible(true);
			FineButton->setVisible(false);
			mode->setVisible(true);
			labelv->setVisible(true);
		}
		else if (buttonThatWasClicked == SaveButton)
		{
			int StateOfWindow = AlertWindow::showNativeDialogBox("Save Settings!", ("Do you want to save the changes in your Xml file?"), true);
			if (StateOfWindow == 1)
				SaveXml();
			else
				return;
		}
		else if (buttonThatWasClicked == FineButton)
		{
			if (FineButton->getToggleState() == false)
			{
				Rusterslider1->sliderMode("Normal");
				Rusterslider2->sliderMode("Normal");
				Rusterslider3->sliderMode("Normal");
				Rusterslider6->sliderMode("Normal");
				RustersliderRot->sliderMode("Normal");
			}
			else if (FineButton->getToggleState() == true)
			{
				FineAmplitude = textEditor->getText().getFloatValue();
				FinePhase = textEditor2->getText().getFloatValue();
				Rusterslider3->sliderMode("Fine");	Rusterslider6->sliderMode("Fine");
				Rusterslider1->sliderMode("Fine");	RustersliderRot->sliderMode("Fine");	Rusterslider2->sliderMode("Fine");
				Rusterslider1->sliderFineModeValues(FineAmplitude, FinePhase, 0.0);
				Rusterslider2->sliderFineModeValues(FineAmplitude, FinePhase, 0.0);
				Rusterslider3->sliderFineModeValues(FineAmplitude, FinePhase, 0.0);
				Rusterslider6->sliderFineModeValues(FineAmplitude, FinePhase, 0.0);
				RustersliderRot->sliderFineModeValues(FineAmplitude, FinePhase, 0.0);
			}
		}
	}

	void sliderValueChanged(Slider* sliderThatWasMoved)
	{
		if (sliderThatWasMoved == Rusterslider1)
			Amp1 = Rusterslider1->getValue();
		else if (sliderThatWasMoved == Rusterslider2)
			Amp2 = Rusterslider2->getValue();
		else if (sliderThatWasMoved == RustersliderRot)
			nev = RustersliderRot->getValue();
		else if (sliderThatWasMoved == Rusterslider3)
			Amp11 = Rusterslider3->getValue();
		else if (sliderThatWasMoved == Rusterslider4)
			Amp12 = Rusterslider4->getValue();
		else if (sliderThatWasMoved == Rusterslider5)
			Amp21 = Rusterslider5->getValue();
		else if (sliderThatWasMoved == Rusterslider6)
			Amp22 = Rusterslider6->getValue();
		else if (sliderThatWasMoved == Delay) {
			VlaDelay = (Delay->getValue()*sampleRate);
			String word;
			if (VlaDelay != 0.000000)
				word << (VlaDelay*((1 / sampleRate) * 1000));
			else
				word = "0";
			word = word + "ms";
			DelayLabel->setText(("Delay:\n" + word), sendNotificationAsync);
		}
	}

	CMarkup InitializeXML()
	{
		String FileName[5] = { "Default1","Default2","Default3","Default4","Default5" };
		mDoc.AddElem("CompleteAlignmentDictionary");
		mDoc.SetAttrib("xmlns:i", "http://www.w3.org/2001/XMLSchema-instance");
		mDoc.SetAttrib("xmlns", "Ariadni.App");
		mDoc.IntoElem();
		for (int i = 0; i< 5; i++)
		{
			mDoc.AddElem("Alignment");
			mDoc.IntoElem();
			mDoc.AddElem("Name", FileName[i].toUTF8());
			mDoc.AddElem("AmplitudeUpperX", "0.3");
			mDoc.AddElem("AmplitudeUpperY", "0.3");
			mDoc.AddElem("Rotation", "0");
			mDoc.AddElem("ScaleUpperXtoLowerX", "0");
			mDoc.AddElem("ScaleUpperXtoLowerY", "0");
			mDoc.AddElem("ScaleUpperYtoLowerX", "0");
			mDoc.AddElem("ScaleUpperYtoLowerY", "0");
			mDoc.OutOfElem();
		}
		mDoc.OutOfElem();
		mDoc.Save(szPath);
		return mDoc;
	}

	CMarkup InitializeHardwareXML()
	{
		mDoc.AddElem("CompleteAlignmentDictionary");
		mDoc.SetAttrib("xmlns:i", "http://www.w3.org/2001/XMLSchema-instance");
		mDoc.SetAttrib("xmlns", "Ariadni.App");
		mDoc.IntoElem();
		mDoc.AddElem("AriadniSettings");
		mDoc.IntoElem();
		mDoc.AddElem("RangesOfFile", 8);
		mDoc.AddElem("Choosen_File", "Default1");
		mDoc.AddElem("IncrementValues");
		mDoc.IntoElem();
		mDoc.AddElem("Fine_Amplitude", "0.0001");
		mDoc.AddElem("Fine_Phase", "0.0");
		mDoc.OutOfElem();
		mDoc.AddElem("HardwareSettings");
		mDoc.IntoElem();
		mDoc.AddElem("Output_Input_Name", "ASIO NanoMEGAS");
		mDoc.AddElem("Sampling_Rate", "96000.000");
		mDoc.AddElem("Buffer_Size", "1024");
		mDoc.OutOfElem();
		mDoc.OutOfElem();
		mDoc.OutOfElem();
		mDoc.Save(szPath2);
		return mDoc;
	}

	CMarkup RenameFile(String oldName, String newName)
	{
		CString FineOldName;
		String IntoString;
		mDoc.Load(szPath);
		mDoc.ResetPos();
		for (int i = 0; i < 5; i++)
		{
			mDoc.FindElem("Alignment");
			mDoc.IntoElem();
			mDoc.FindElem("Name");
			FineOldName = mDoc.GetElemContent();
			IntoString << FineOldName;
			if (IntoString == oldName)
				mDoc.SetElemContent(newName.toUTF8());
			IntoString.clear();
			mDoc.OutOfElem();
		}
		mDoc.Save(szPath);
		return 0;
	}

	void comboBoxChanged(ComboBox* comboBoxThatHasChanged)
	{
		if (comboBoxThatHasChanged == FileSelection)
		{
			LoadXml();
			label4->setText("Mode: " + FileSelection->getText(), sendNotificationAsync);
		}
	}

	CMarkup LoadXml()
	{
		int IndexOfFile = FileSelection->getSelectedId();
		mDoc.Load(szPath);
		mDoc.ResetPos();
		mDoc.FindElem("CompleteAlignmentDictionary");
		mDoc.IntoElem();
		int i = 0;
		while (i++ < IndexOfFile)
		{
			mDoc.FindElem("Alignment");
		}
		mDoc.IntoElem();
		mDoc.FindElem("AmplitudeUpperX");
		Rusterslider1->setValue(double(atof(mDoc.GetElemContent())));
		mDoc.FindElem("AmplitudeUpperY");
		Rusterslider2->setValue(double(atof(mDoc.GetElemContent())));
		mDoc.FindElem("Rotation");
		RustersliderRot->setValue(double(atof(mDoc.GetElemContent())));
		mDoc.FindElem("ScaleUpperXtoLowerX");
		Rusterslider3->setValue(double(atof(mDoc.GetElemContent())));
		mDoc.FindElem("ScaleUpperXtoLowerY");
		Rusterslider6->setValue(double(atof(mDoc.GetElemContent())));
		mDoc.FindElem("ScaleUpperYtoLowerX");
		Rusterslider4->setValue(double(atof(mDoc.GetElemContent())));
		mDoc.FindElem("ScaleUpperYtoLowerY");
		Rusterslider5->setValue(double(atof(mDoc.GetElemContent())));
		mDoc.OutOfElem();
		mDoc.OutOfElem();
		return 0;
	}

	CMarkup StartUpFile()
	{
		MDocSettings.Load(szPath2);
		MDocSettings.ResetPos();
		MDocSettings.FindElem("CompleteAlignmentDictionary");
		MDocSettings.IntoElem();
		MDocSettings.FindElem("AriadniSettings");
		MDocSettings.IntoElem();
		MDocSettings.FindElem("Choosen_File");
		String FileNameList;
		FileNameList << MDocSettings.GetElemContent();
		label4->setText("Mode: " + FileNameList, sendNotificationAsync);
		mDoc.OutOfElem();
		mDoc.OutOfElem();
		for (int i = 0; i < 5; i++)
		{
			FileSelection->setSelectedId(i + 1);
			if (FileNameList == FileSelection->getText())
				return 0;
		}
		return 0;
	}

	CMarkup SaveXml()
	{
		String Val;
		Val = "";
		int IndexOfFile = FileSelection->getSelectedId();
		mDoc.Load(szPath);
		mDoc.ResetPos();
		mDoc.FindElem("CompleteAlignmentDictionary");
		mDoc.IntoElem();
		int o = 0;
		while (o++ < IndexOfFile)
		{
			mDoc.FindElem("Alignment");
		}
		mDoc.IntoElem();
		mDoc.FindElem("AmplitudeUpperX");
		Val << Rusterslider1->getValue();
		mDoc.SetElemContent(Val.toUTF8());
		mDoc.FindElem("AmplitudeUpperY");
		Val = ""; Val << Rusterslider2->getValue();
		mDoc.SetElemContent(Val.toUTF8());
		mDoc.FindElem("Rotation");
		Val = ""; Val << RustersliderRot->getValue();
		mDoc.SetElemContent(Val.toUTF8());
		mDoc.FindElem("ScaleUpperXtoLowerX");
		Val = ""; Val << Rusterslider3->getValue();
		mDoc.SetElemContent(Val.toUTF8());
		mDoc.FindElem("ScaleUpperXtoLowerY");
		Val = ""; Val << Rusterslider6->getValue();
		mDoc.SetElemContent(Val.toUTF8());
		mDoc.FindElem("ScaleUpperYtoLowerX");
		Val = ""; Val << Rusterslider4->getValue();
		mDoc.SetElemContent(Val.toUTF8());
		mDoc.FindElem("ScaleUpperYtoLowerY");
		Val = ""; Val << Rusterslider5->getValue();
		mDoc.SetElemContent(Val.toUTF8());
		mDoc.OutOfElem();
		mDoc.OutOfElem();
		mDoc.Save(szPath);
		return 0;
	}

	void audioWindow()
	{
		bool showMidiInputOptions = false;
		bool showMidiOutputSelector = false;
		bool showChannelsAsStereoPairs = false;
		String LastFile;
		LastFile = FileSelection->getText();
		bool bLoadResult = MDocSettings.Load(szPath2);
		if (InputDeviceName.isEmpty() || OutputDeviceName.isEmpty()) {/*Break;*/ }
		else
		{
			MDocSettings.ResetPos();
			MDocSettings.FindElem("CompleteAlignmentDictionary");
			MDocSettings.IntoElem();
			MDocSettings.FindElem("AriadniSettings");
			MDocSettings.IntoElem();
			MDocSettings.FindElem("Choosen_File");
			MDocSettings.SetElemContent(LastFile.toUTF8());
			MDocSettings.FindElem("IncrementValues");
			MDocSettings.IntoElem();
			MDocSettings.FindElem("Fine_Amplitude");
			MDocSettings.SetElemContent(textEditor->getText().toUTF8());
			String DelayVal;
			DelayVal << Delay->getValue();
			MDocSettings.FindElem("Fine_Phase");
			MDocSettings.SetElemContent(DelayVal.toUTF8());
			MDocSettings.OutOfElem();
			MDocSettings.FindElem("HardwareSettings");
			MDocSettings.IntoElem();
			MDocSettings.FindElem("Output_Input_Name");
			MDocSettings.SetElemContent(OutputDeviceName.toUTF8());
			MDocSettings.FindElem("Sampling_Rate");
			String str = ""; str = "96000.000";
			MDocSettings.SetElemContent(str.toUTF8());
			MDocSettings.FindElem("Buffer_Size");
			str = ""; str << BufferSize;
			MDocSettings.SetElemContent(str.toUTF8());
			MDocSettings.OutOfElem();
			MDocSettings.OutOfElem();
			MDocSettings.OutOfElem();
			MDocSettings.Save(szPath2);
		}
	}

	void InitializeAudioDevice()
	{
		if (MDocSettings.Load(szPath2) == true) {
			MDocSettings.ResetPos();
			MDocSettings.FindElem("CompleteAlignmentDictionary");
			MDocSettings.IntoElem();
			MDocSettings.FindElem("AriadniSettings");
			MDocSettings.IntoElem();
			MDocSettings.FindElem("IncrementValues");
			MDocSettings.IntoElem();
			MDocSettings.FindElem("Fine_Amplitude");
			textEditor->setText(String(MDocSettings.GetElemContent()), true);
			MDocSettings.FindElem("Fine_Phase");
			Delay->setValue(atof(MDocSettings.GetElemContent()));
			MDocSettings.OutOfElem();
			MDocSettings.FindElem("HardwareSettings");
			MDocSettings.IntoElem();
			MDocSettings.FindElem("Output_Input_Name");
			OutputDeviceName = String(MDocSettings.GetElemContent());
			InputDeviceName = OutputDeviceName;
			MDocSettings.FindElem("Sampling_Rate");
			SLR = atoi(MDocSettings.GetElemContent());
			MDocSettings.FindElem("Buffer_Size");
			Buffer = atoi(MDocSettings.GetElemContent());
			MDocSettings.OutOfElem();
			MDocSettings.OutOfElem();
			MDocSettings.OutOfElem();
			SLR = 96000.000;
			WorkingAudioCardSetup.inputDeviceName = InputDeviceName;
			WorkingAudioCardSetup.outputDeviceName = OutputDeviceName;
			WorkingAudioCardSetup.sampleRate = SLR;
			WorkingAudioCardSetup.bufferSize = Buffer;
			WorkingAudioCardSetup.outputChannels.setRange(10, 12, true);
			WorkingAudioCardSetup.inputChannels.setRange(6, 6, true);
			sampleRate = SLR;
			BufferSize = Buffer;
			//frequency;
			SR = sampleRate;
			cookFrequency();
			setAudioChannels(6, 10, OutputDeviceName, WorkingAudioCardSetup);
			FineAmplitude = textEditor->getText().getFloatValue();
			FinePhase = textEditor2->getText().getFloatValue();
		}
	}

private:
	int Buffer, SLR;
	float VlaDelay;
	String OutputDeviceName, InputDeviceName;
	AudioDeviceManager::AudioDeviceSetup WorkingAudioCardSetup;
	ScopedPointer<Slider> Rusterslider1, Rusterslider2, Rusterslider3, Rusterslider4, Rusterslider5, Rusterslider6, RustersliderRot, Delay;
	ScopedPointer<Label> label, label2, labelf, labelfa, labelfp, DelayLabel;
	ScopedPointer<Label> label3, label4, label5, label6, label7, label8, labelv, mode;
	ScopedPointer<ToggleButton> FineButton;
	CMarkup MDocSettings; int Tamp, TestingVal;
	ScopedPointer<ComboBox> FileSelection;
	double FineAmplitude, FinePhase;
	ScopedPointer<TextEditor> textEditor, textEditor2;
	CMarkup mDoc;
	Path internalPath1;
	Path internalPath2;
	Path internalPath3;
	Path internalPath4;
	Path internalPath5;
	double Amp1, Amp2, nev, Amp11, Amp12, Amp22, Amp21;
	ScopedPointer<GroupComponent> groupComponent, groupComponent2;
	ScopedPointer<TextButton> BButton, SettingButton, SaveButton, textButton, textButton2;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainContentComponent)
};

class MainWindow : public DocumentWindow
{
public:
	MainWindow(String name) : DocumentWindow(name,
		Colours::white,
		5)
	{
		scape = new MainContentComponent();
		setUsingNativeTitleBar(true);
		setContentOwned(scape, true);
		centreWithSize(getWidth(), getHeight());
		setVisible(false);
	}

	void closeButtonPressed() override
	{

		ExitStatus = 1;
		if (ExitStatus == 1) {
			int StateOfWindow = AlertWindow::showNativeDialogBox("Exit from Acom Scanning Patch!", ("Do you want to save the changes in your Xml file?"), true);
			if (StateOfWindow == 1)
				scape->SaveXml();
		}
		setVisible(false);
	}
	void reloadFile() { scape->LoadXml(); }
	ScopedPointer<MainContentComponent> scape;
private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

MainWindow* MainWindowContent;

void setOffsetX(const double offset)
{
	AstarOffset1 = offset;
}

void setOffsetY(const double offset)
{
	AstarOffset2 = offset;
}
 
BOOL WINAPI DllMain (HINSTANCE instance, DWORD dwReason, LPVOID)
{
		if (dwReason == DLL_PROCESS_ATTACH)
		{
			//Process::setCurrentModuleInstanceHandle (instance);
				//MessageBoxW(NULL, L"Dll is Loaded!", L"Done!", MB_ICONEXCLAMATION | MB_OK);
		}
		/*else if (dwReason == DLL_PROCESS_DETACH)
		{
			if (MainWindowContent != NULL)
			{
				//deleteAndZero(MainWindowContent);
				//shutdownJuce_GUI();
			}
		}
		else if (dwReason == DLL_THREAD_DETACH)
		{
			MessageBoxW(NULL, L"Dll is Unoaded!", L"Done!", MB_ICONEXCLAMATION | MB_OK);
		}*/
return TRUE;
}
__declspec (dllexport)  int  InitAriadni(TAriadniCallBackFunc i)
{
	if (MainWindowContent == nullptr)
	{
		CallBack = i;
		initialiseJuce_GUI();
		MainWindowContent = new MainWindow("ACOM Scanning Patch");
		MainWindowContent->setVisible(false);
		ArStatus = 1;
		CallBack(ArStatus, 0.0);
		if (MainWindowContent->scape->Temp == 1)//P2000 is connected
			return (int)MainWindowContent;
		else
			return -50;
	}
	else {
		ArStatus = -1;
		CallBack(ArStatus, 0.0);
		return ArStatus;
	}
}

__declspec (dllexport)   int ShowGui(bool State) {
	if (State)
	{
		MainWindowContent->setVisible(true);
		VisibilityState = true;
		CallBack(1, 0);
		MainWindowContent->reloadFile();
	}
	else if (!State)
	{
		MainWindowContent->setVisible(false);
		VisibilityState = false;
		CallBack(0, 0);
	}
	return 1;
}

__declspec (dllexport) int SetLineScanData(TDestChannels Channels, int EventCount, TScanData * Data)
{
	int state = 0;
	int steps = 0;
	if (waiting != true) {
		Temp = 0;
		KeeperArray = 0;
		Keeper = 0;
		k = 0;
		int nextEvent; nextEvent = 0;

		int Val = 0;
		double Ar[2][512];
		for (int i = 0; i < 2; i++)
			for (int j = 0; j < 512; j++)
				Ar[i][j] = 0.0;
		setRuster(Ar);
		ChDest = (Channels & 0xFF);
		ScanDuration = 0;
		state = 0; DurationOfLine = 0;
		if (ChDest > 0 && ChDest < 16)
		{
			TimeDataScan = new double[EventCount];
			Ch1DataScan = new double[EventCount];
			Ch2DataScan = new double[EventCount];
			Ch3DataScan = new double[EventCount];
			Ch4DataScan = new double[EventCount];
			Ch5DataScan = new double[EventCount];
			Ch6DataScan = new double[EventCount];
			TimeScanPerEvent = new int[EventCount];
			//ChCameraDataScan = new double[EventCount];
			ChCameraDataScan = new double[Data[EventCount - 1].Time];
			ChCameraDataScan9 = new double[Data[EventCount - 1].Time];
			count = EventCount;
			while (state < EventCount)
			{
				Ch1DataScan[k] = Data[state].BDUY;
				Ch2DataScan[k] = Data[state].BDUX;
				Ch3DataScan[k] = Data[state].BDLY;
				Ch4DataScan[k] = Data[state].BDLX;
				Ch5DataScan[k] = Data[state].IDUY;
				Ch6DataScan[k] = Data[state].IDUX;
				TimeScanPerEvent[k] = Data[state].Time;
				//ChCameraDataScan[k] = Data[state].Shutter;
				for (int i = Val; i < Data[state].Time; i++)
					ChCameraDataScan[i] = Data[state].Shutter;
				Val = Data[state].Time;

				k = k + 1;
				state = state + 1;
			}

			for (int o = 0; o < EventCount; o = o + 2)
				TimeDataScan[o] = abs(Data[0].Time - Data[1].Time);
			for (int o = 1; o < EventCount; o = o + 2)
				TimeDataScan[o] = Data[0].Time;

			for (int j = 0; j < EventCount; j = j + 2)
			{
				for (int i = 0; i < TimeDataScan[j]; i++)
				{
					ChCameraDataScan9[steps] = Data[1].Shutter;
					steps++;
				}
				for (int i = 0; i < TimeDataScan[j + 1]; i++)
				{
					ChCameraDataScan9[steps] = Data[0].Shutter;
					steps++;
				}

			}
			DurationOfLine = Data[state - 1].Time;
		}
		waiting = true;
		ArStatus = 0;
		CallBack(ArStatus, 0.0);
	}
	else state = -100;
	return state;
}

__declspec (dllexport) void AbortLineScan()
{
	DurationOfLine = 0;
	if (Ch1DataScan != NULL) {
		delete[] Ch1DataScan; delete[] Ch2DataScan; delete[] Ch3DataScan; Ch1DataScan = NULL; Ch2DataScan = NULL; Ch3DataScan = NULL;
		delete[] Ch4DataScan; delete[] Ch5DataScan; delete[] Ch6DataScan; delete[] ChCameraDataScan; delete[] ChCameraDataScan9;
		Ch4DataScan = NULL; Ch5DataScan = NULL; Ch6DataScan = NULL; ChCameraDataScan = NULL; ChCameraDataScan9 = NULL;
	}
	waiting = false;
}

__declspec (dllexport) int ReleaseAriadni()
{
	if (MainWindowContent != NULL)
	{
		if (MainWindowContent->isVisible())
			VisibilityState = true;
		else if (!MainWindowContent->isVisible())
			VisibilityState = false;
		ExitStatus = 2;
		MainWindowContent->removeFromDesktop();
		MainWindowContent->deleteAllChildren();
		shutdownJuce_GUI();
		MainWindowContent = NULL;
		ArStatus = 0;
		CallBack(ArStatus, 0.0);
		return 0;
	}
	else
		return -1;
}

__declspec (dllexport) int  SetScanOutline(/*TDestChannels TDC,*/ double Array[2][512])
{
	int result = 0;
	setOffsetX(0);
	setOffsetY(0);
	setRuster(Array);
	if (MainWindowContent != NULL)
	{
		result = 1;
		CallBack(result, 0);
	}
	else
	{
		result = -1;
		CallBack(result, 0);
	}
	return result;
}

__declspec (dllexport) int ShowScanOutline(bool States)
{
	if (!States) {
		EnableSCan = 0;
	}
	else if (States) {
		EnableSCan = 1;
	}
	return EnableSCan;
}

__declspec (dllexport) double MoveBeam(double x, double y)
{
	int result = 0;
	double ArrEmpty[2][512];
	for (int j = 0; j<2; j++)
		for (int i = 0; i<512; i++)
			ArrEmpty[j][i] = 0.0000;
	setRuster(ArrEmpty);
	setOffsetX(x);
	setOffsetY(y);
	
	if (MainWindowContent != NULL)
	{
		if ((x >= -1.0 && x <= 1.0) && (y >= -1.0 && y <= 1.0))
			result = 1;
		else
			result = -1;
		CallBack(result, 0);		
	}
	else
	{
		result = -1;
		CallBack(result, 0);
	}
	PlayTones = true;
	return result;
}

__declspec (dllexport)  int GetAriadniStatus() {
	if (waiting == true)
		ArStatus = 0;
	else if (waiting == false)
		ArStatus = 1;
	CallBack(ArStatus, 0.0);
	return 0;
}

Component* createMainContentComponent() { return new MainContentComponent(); }