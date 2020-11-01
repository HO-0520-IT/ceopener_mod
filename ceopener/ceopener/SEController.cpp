#include "SEController.h"

#include <mmsystem.h>
#include <knceutil.h>

using namespace std;

SEController::SoundEffect SEController::m_MUON;

SEController::SEController(void) {
	tstring sndDir = KnceUtil::getCurrentDirectory() + _T("\\sounds");

	m_MUON.setVerb(_T("MUON"));
	m_MUON.setFileName(sndDir + _T("\\muon.wav"));
}

SEController::~SEController(void) {
	releaseSoundEffects();
}

bool SEController::playSE(tstring verb) {
	SoundEffect *se = this->findByVerb(verb);
	if(se == NULL) {
		return false;
	}
	else
		se->playSE();

	return true;
}

void SEController::releaseSoundEffects(void) {
	for(unsigned int i = 0; i < m_SoundEffects.size(); i++) {
		delete m_SoundEffects[i];
	}
}

SEController::SoundEffect::SoundEffect(void) {
	this->m_lpSnd = NULL;
	this->m_enabled = false;
	this->m_playMode = SND_ASYNC | SND_MEMORY;
}

SEController::SoundEffect::~SoundEffect(void) {
	this->m_lpSnd != NULL ? delete this->m_lpSnd : 1;
}

void SEController::SoundEffect::playSE(void) {
	if(this->m_enabled) {
		HINSTANCE hInst = GetModuleHandle(NULL);

		if(this->verb() != _T("MUON")) {
			SEController::m_MUON.playSE();
			Sleep(100);
		}

		if(this->m_playMode & SND_MEMORY)
			PlaySound((LPTSTR)m_lpSnd, hInst, this->m_playMode);
		else if(this->m_playMode & SND_FILENAME)
			PlaySound(m_fileName.c_str(), hInst, this->m_playMode);
	}
}

void SEController::SoundEffect::reloadSnd(void) {
	DWORD dwFileSize, dwReadSize;

	HANDLE hFile = CreateFile(m_fileName.c_str(), GENERIC_READ, NULL, NULL,
		OPEN_EXISTING, NULL, NULL);
	if(hFile == INVALID_HANDLE_VALUE) {
		m_lpSnd = NULL;
		return;
	}
	dwFileSize = GetFileSize(hFile, NULL);
	m_lpSnd != NULL ? delete m_lpSnd : 1;
	this->m_lpSnd = new BYTE[dwFileSize];
	ReadFile(hFile, (LPVOID)m_lpSnd, dwFileSize, &dwReadSize, NULL);

	if(dwReadSize < dwFileSize) {
		delete m_lpSnd;
		m_lpSnd = NULL;
		return;
	}
	CloseHandle(hFile);
}

