#include <ddraw.h>
#include <pathcch.h>

#include "pch.h"
#include "MyDialog.h"
#include "DirectX.h"
#include "IniFile.h"
#include "resource.h"

// Constructor.
CMyDialog::CMyDialog(UINT resID) : CDialog(resID)
{
}

// Called when the dialog window is destroyed.
void CMyDialog::OnDestroy()
{
    // End the application
    ::PostQuitMessage(0);
}

// Process the dialog's window messages.
INT_PTR CMyDialog::DialogProc(UINT msg, WPARAM wparam, LPARAM lparam)
{
    try
    {
        m_resizer.HandleMessage(msg, wparam, lparam);

        switch (msg)
        {
            case WM_SIZE:
                // workaround for https://github.com/DavidNash2024/Win32xx/issues/11
                if (wparam == 0)
                    m_resizer.OnAfterDpiChange();
                [[fallthrough]];
            case WM_SIZING:
            {
                CRect rc = GetButtonBoxRect();
                InvalidateRect(rc);
            }
            case WM_CTLCOLORSTATIC:
                return reinterpret_cast<INT_PTR>(GetStockObject(WHITE_BRUSH));
        }

        // Pass unhandled messages on to parent DialogProc.
        return DialogProcDefault(msg, wparam, lparam);
    }

    catch (const CException& e)
    {
        // Display the exception and continue.
        CString str1;
        str1 << e.GetText() << L'\n' << e.GetErrorString();
        CString str2;
        str2 << "Error: " << e.what();
        ::MessageBox(nullptr, str1, str2, MB_ICONERROR);
    }

    // Catch all unhandled std::exception types.
    catch (const std::exception& e)
    {
        // Display the exception and continue.
        CString str1 = e.what();
        ::MessageBox(nullptr, str1, L"Error: std::exception", MB_ICONERROR);
    }

    return 0;
}

// Handle the command notifications from controls.
BOOL CMyDialog::OnCommand(WPARAM wparam, LPARAM)
{
    UINT id = LOWORD(wparam);
    UINT code = HIWORD(wparam);
    switch (id)
    {
        case IDC_COMBO1:
            if (code == CBN_SELCHANGE)
            {
                UpdateGraphicsModes();
                return TRUE;
            }
            break;
        case IDC_CHECK1:
            return OnToggleWindowed();
        default:
            return FALSE;
    }
    return FALSE;
}


// Called before the dialog is displayed.
BOOL CMyDialog::OnInitDialog()
{
    // Set the application icon
    SetIconLarge(IDW_MAIN);
    SetIconSmall(IDW_MAIN);

    // Attach the controls to our CWnd member objects.
    AttachItem(IDC_COMBO1, m_comboAdapter);
    AttachItem(IDC_COMBO2, m_comboResolution);
    AttachItem(IDC_CHECK1, m_checkWindowed);
    AttachItem(IDC_COMBO3, m_comboAntialiasing);
    AttachItem(IDC_COMBO4, m_comboFiltering);
    AttachItem(IDOK, m_buttonOK);
    AttachItem(IDCANCEL, m_buttonCancel);

    // Initialize DirectX
    dx = new DirectX();

    // Initialize IniFile
    std::filesystem::path mainIniFileName;
    PWSTR savedGamesPath{};

    if (SHGetKnownFolderPath(FOLDERID_SavedGames, KF_FLAG_CREATE, nullptr, &savedGamesPath) == S_OK) {
        mainIniFileName = savedGamesPath;

        mainIniFileName += "\\Tonic Trouble";
        std::filesystem::create_directories(mainIniFileName); // create directory if it doesn't exist

        mainIniFileName += "\\ubi.ini";
        CoTaskMemFree(savedGamesPath);
    }
    iniFile = new IniFile(mainIniFileName, {L"C:\\Windows\\Ubisoft\\Ubi.ini", L"Ubi.ini"});

    for (auto& deviceName : dx->GetDeviceNames()) {
        m_comboAdapter.InsertString(m_comboAdapter.GetCount(), deviceName);
    }
    int selectionIndex = m_comboAdapter.FindString(-1, iniFile->gliDriver);
    m_comboAdapter.SetCurSel(selectionIndex != CB_ERR ? selectionIndex : 0);

    UpdateGraphicsModes();

    CString modeString;
    modeString.Format(L"%i×%i×%i", iniFile->width, iniFile->height, iniFile->bpp);
    selectionIndex = m_comboResolution.FindString(-1, modeString);
    m_comboResolution.SetCurSel(selectionIndex != CB_ERR ? selectionIndex : 0);

    m_checkWindowed.SetCheck(iniFile->windowed);


    selectionIndex = m_comboAntialiasing.FindString(-1, ToCString(iniFile->antialiasing));
    m_comboAntialiasing.SetCurSel(selectionIndex != CB_ERR ? selectionIndex : 0);

    selectionIndex = m_comboFiltering.FindString(-1, ToCString(iniFile->anisotropy));
    m_comboFiltering.SetCurSel(selectionIndex != CB_ERR ? selectionIndex : 0);

    m_resizer.Initialize(*this, CRect(0, 0, 400, 200));
    m_resizer.AddChild(m_comboAdapter, CResizer::topright, RD_STRETCH_WIDTH);
    m_resizer.AddChild(m_comboResolution, CResizer::topright, RD_STRETCH_WIDTH);
    m_resizer.AddChild(m_comboAntialiasing, CResizer::topright, RD_STRETCH_WIDTH);
    m_resizer.AddChild(m_comboFiltering, CResizer::topright, RD_STRETCH_WIDTH);
    m_resizer.AddChild(m_buttonOK, CResizer::bottomright, 0);
    m_resizer.AddChild(m_buttonCancel, CResizer::bottomright, 0);
    m_resizer.RecalcLayout();

    return TRUE;
}

BOOL CMyDialog::OnEraseBkgnd(CDC &dc) {
    // draw main background
    CRect rc = GetClientRect();
    dc.FillRect(rc, GetSysColorBrush(COLOR_WINDOW));

    // draw button box background
    rc = GetButtonBoxRect();
    dc.FillRect(rc, GetSysColorBrush(COLOR_BTNFACE));
    dc.DrawEdge(rc, BDR_SUNKENOUTER, BF_TOP);
    return TRUE;
}

// Called when the OK button or Enter key is pressed.
void CMyDialog::OnOK()
{
    // get adapter
    int len = m_comboAdapter.GetLBTextLen(m_comboAdapter.GetCurSel());
    m_comboAdapter.GetLBText(m_comboAdapter.GetCurSel(), iniFile->gliDriver.GetBuffer(len));
    iniFile->gliDriver.ReleaseBuffer();

    // get mode
    CString modeString;
    len = m_comboResolution.GetLBTextLen(m_comboResolution.GetCurSel());
    m_comboResolution.GetLBText(m_comboResolution.GetCurSel(), modeString.GetBuffer(len));
    modeString.ReleaseBuffer();
    swscanf_s(modeString, L"%i×%i×%i", &iniFile->width, &iniFile->height, &iniFile->bpp);

    iniFile->windowed = m_checkWindowed.GetCheck() == BST_CHECKED;
    iniFile->antialiasing = m_comboAntialiasing.GetItemData(m_comboAntialiasing.GetCurSel());
    iniFile->anisotropy = m_comboFiltering.GetItemData(m_comboFiltering.GetCurSel());

    iniFile->WriteSettingsToFile();
    CDialog::OnOK();
}

CRect CMyDialog::GetButtonBoxRect()
{
    // stupid hacky way of converting dialog units to pixels in just one dimension
    CRect heightRect {0, 0, 0, 28};
    MapDialogRect(heightRect);
    LONG height = heightRect.bottom;

    CRect clientRect = GetClientRect();
    clientRect.top = clientRect.bottom - height;
    return clientRect;
}

void CMyDialog::UpdateGraphicsModes()
{
    CString originalSelection;
    int originalSelectionLen;
    int newSelectionIndex;

    originalSelectionLen = m_comboResolution.GetLBTextLen(m_comboResolution.GetCurSel());
    if (originalSelectionLen != CB_ERR) {
        m_comboResolution.GetLBText(m_comboResolution.GetCurSel(), originalSelection.GetBuffer(originalSelectionLen));
        originalSelection.ReleaseBuffer(originalSelectionLen);
    }
    m_comboResolution.ResetContent();
    auto displayModes = dx->GetDisplayModes(m_comboAdapter.GetCurSel());
    //for (auto& mode : displayModes) {
    for (int i = 0; i < displayModes.size(); i++) {
        CString modeString;
        modeString.Format(L"%i×%i×%i", displayModes[i].width, displayModes[i].height, displayModes[i].bitsPerPixel);
        m_comboResolution.InsertString(m_comboResolution.GetCount(), modeString);
        m_comboResolution.SetItemData(m_comboResolution.GetCurSel(), i);
    }
    newSelectionIndex = m_comboResolution.FindString(-1, originalSelection);
    if (newSelectionIndex != CB_ERR) {
        m_comboResolution.SetCurSel(newSelectionIndex);
    }
    else {
        m_comboResolution.SetCurSel(0);
    }

    originalSelectionLen = m_comboAntialiasing.GetLBTextLen(m_comboAntialiasing.GetCurSel());
    if (originalSelectionLen != CB_ERR) {
        m_comboAntialiasing.GetLBText(m_comboAntialiasing.GetCurSel(), originalSelection.GetBuffer(originalSelectionLen));
        originalSelection.ReleaseBuffer(originalSelectionLen);
    }
    m_comboAntialiasing.ResetContent();
    m_comboAntialiasing.AddString(L"None"); // always supported
    auto antiAliasingModes = dx->GetAntiAliasingModes(m_comboAdapter.GetCurSel(), m_checkWindowed.GetCheck());
    for (auto& mode : antiAliasingModes) {
        CString modeString;
        modeString.Format(L"%i×", mode);
        m_comboAntialiasing.InsertString(m_comboAntialiasing.GetCount(),modeString);
        m_comboAntialiasing.SetItemData(m_comboAntialiasing.GetCount() - 1, mode);
    }
    newSelectionIndex = m_comboAntialiasing.FindString(-1, originalSelection);
    if (newSelectionIndex != CB_ERR) {
        m_comboAntialiasing.SetCurSel(newSelectionIndex);
    }
    else {
        m_comboAntialiasing.SetCurSel(0);
    }

    originalSelectionLen = m_comboFiltering.GetLBTextLen(m_comboFiltering.GetCurSel());
    if (originalSelectionLen != CB_ERR) {
        m_comboFiltering.GetLBText(m_comboFiltering.GetCurSel(), originalSelection.GetBuffer(originalSelectionLen));
        originalSelection.ReleaseBuffer(originalSelectionLen);
    }
    m_comboFiltering.ResetContent();
    m_comboFiltering.AddString(L"None");
    DWORD maxAnisotropy = dx->GetMaxAnisotropy(m_comboAdapter.GetCurSel());
    for (auto i = 2; i <= maxAnisotropy; i*=2)
    {
        CString anisotropyString;
        anisotropyString.Format(L"%i×", i);
        m_comboFiltering.InsertString(m_comboFiltering.GetCount(),anisotropyString);
        m_comboFiltering.SetItemData(m_comboFiltering.GetCount() - 1, i);
    }
    newSelectionIndex = m_comboFiltering.FindString(-1, originalSelection);
    if (newSelectionIndex != CB_ERR) {
        m_comboFiltering.SetCurSel(newSelectionIndex);
    }
    else {
        m_comboFiltering.SetCurSel(0);
    }
}

BOOL CMyDialog::OnToggleWindowed()
{
    UpdateGraphicsModes();
    return TRUE;
}
