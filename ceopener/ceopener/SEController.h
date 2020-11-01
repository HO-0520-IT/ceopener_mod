#ifndef SECONTROLLER_H_
#define SECONTROLLER_H_

#include <windows.h>
#include <vector>
#include <string>


#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif


class SEController {
public:
	class SoundEffect {
	public:
		SoundEffect(void);
		virtual ~SoundEffect(void);
		std::tstring verb(void) const { return m_verb; }
		void setVerb(std::tstring verb) { m_verb = verb; }
		std::tstring fileName(void) const { return m_fileName; }
		void setFileName(std::tstring fileName) { m_fileName = fileName; reloadSnd(); }
		bool isEnabled(void) const { return m_enabled; }
		void setEnabled(bool enabled) { m_enabled = enabled; }
		UINT playMode(void) const { return m_playMode; }
		void setPlayMode(UINT mode) { 
			m_playMode = mode;
			if(mode & SND_MEMORY) {
				reloadSnd();
			}
			else if(mode & SND_FILENAME) {
				delete m_lpSnd;
				m_lpSnd = NULL;
			}
		}
		void playSE(void);
		SoundEffect &operator=(std::tstring fileName) {
			this->setFileName(fileName);
			return *this;
		}
		operator std::tstring() {
			return m_fileName;
		}
	private:
		std::tstring m_verb;
		std::tstring m_fileName;
		UINT m_playMode;
		bool m_enabled;
		BYTE* m_lpSnd;
		void reloadSnd();
	};
	SEController(void);
	virtual ~SEController(void);

	SoundEffect * findByVerb(std::tstring verb) {
		for(unsigned int i = 0; i < m_SoundEffects.size(); i++) {
			if(verb == m_SoundEffects[i]->verb())
				return m_SoundEffects[i];
		}
		
		return NULL;
	}

	

	SoundEffect &operator[](std::tstring verb) {
		SoundEffect *found = this->findByVerb(verb);
		if(found == NULL) {
			SoundEffect *se = new SoundEffect;
			se->setVerb(verb);
			m_SoundEffects.push_back(se);
			return *se;
		}
		else
			return *found;
	}

	std::vector<SoundEffect*> getSoundEffects(void) { return m_SoundEffects; }
	bool playSE(std::tstring verb);

	static SoundEffect m_MUON;
private:
	SoundEffect m_dummySE;
	std::vector<SoundEffect*> m_SoundEffects;
	void releaseSoundEffects(void);
};

#endif