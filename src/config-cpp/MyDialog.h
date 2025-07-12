#ifndef MYDIALOG_H
#define MYDIALOG_H
#include "DirectX.h"
#include "IniFile.h"

class CMyDialog : public CDialog
{
public:
    CMyDialog(UINT resID);
    virtual ~CMyDialog() override = default;

protected:
    // Virtual functions that override base class functions
    virtual void    OnDestroy() override;
    virtual BOOL    OnInitDialog() override;
    virtual INT_PTR DialogProc(UINT msg, WPARAM wparam, LPARAM lparam) override;
    virtual BOOL    OnCommand(WPARAM wparam, LPARAM lparam) override;
    virtual BOOL    OnEraseBkgnd(CDC &dc) override;
    virtual void    OnOK() override;

private:
    CMyDialog(const CMyDialog&) = delete;
    CMyDialog& operator=(const CMyDialog&) = delete;

    CRect GetButtonBoxRect();
    void UpdateGraphicsModes();

    // Command handlers
    BOOL OnToggleWindowed();

    // resizer
    CResizer m_resizer;

    // Member variables
    CComboBox m_comboAdapter;
    CComboBox m_comboResolution;
    CButton m_checkWindowed;
    CComboBox m_comboAntialiasing;
    CComboBox m_comboFiltering;
    CButton m_buttonOK;
    CButton m_buttonCancel;

    DirectX *dx;
    IniFile *iniFile;
};

#endif //MYDIALOG_H
