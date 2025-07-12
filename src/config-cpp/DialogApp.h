#ifndef DIALOGAPP_H
#define DIALOGAPP_H

#include "MyDialog.h"

class CDialogApp : public CWinApp
{
public:
    CDialogApp();
    virtual ~CDialogApp() override = default;

protected:
    // Virtual functions that override base class functions
    virtual BOOL InitInstance() override;

private:
    CDialogApp(const CDialogApp&) = delete;
    CDialogApp& operator=(const CDialogApp&) = delete;

    CMyDialog m_myDialog;
};


#endif // define DIALOGAPP_H
