#ifndef CTRLDLG_H
#define CTRLDLG_H

#include <windows.h>

class ControlDialogParams {
public:
    ControlDialogParams();
    virtual ~ControlDialogParams();
    int getBrightness() const { return m_brightness; }
    void setBrightness(int bright) { m_brightness = bright; }
    int getClockGear() const { return m_clockGear; }
    void setClockGear(int gear) { m_clockGear = gear; }
    bool isBacklightDimmingEnabled() const
        { return m_isBacklightDimmingEnabled; }
    void setBacklightDimmingEnabled(bool enabled)
        { m_isBacklightDimmingEnabled = enabled; }
    bool isPowerDownEnabled() const { return m_isPowerDownEnabled; }
    void setPowerDownEnabled(bool enabled)
        { m_isPowerDownEnabled = enabled; }
    bool isMrSensorEnabled() const { return m_isMrSensorEnabled; }
    void setMrSensorEnabled(bool enabled)
        { m_isMrSensorEnabled = enabled; }

private:
    int m_brightness;
    int m_clockGear;
    bool m_isBacklightDimmingEnabled;
    bool m_isPowerDownEnabled;
    bool m_isMrSensorEnabled;
};

bool showControlDialog(HWND hOwnerWindow, ControlDialogParams &params);

#endif  // CTRLDLG_H
