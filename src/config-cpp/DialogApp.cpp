#include "pch.h"
#include "DialogApp.h"
#include "resource.h"

// Constructor.
CDialogApp::CDialogApp() : m_myDialog(IDD_DIALOG1)
{
}

// Called when the application starts.
BOOL CDialogApp::InitInstance()
{
    // Display the modal dialog.
    m_myDialog.DoModal();   // throws a CWinException on failure.

    return TRUE;
}

