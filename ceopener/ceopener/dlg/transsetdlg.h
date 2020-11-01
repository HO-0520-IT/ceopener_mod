#ifndef TRANSSETDLG_H_
#define TRANSSETDLG_H_

#include <vector>
#include <utility>
#include <map>
#include <string>
#include <windows.h>


#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using std::tstring;

typedef std::vector<std::pair<std::tstring, std::map<UINT, UINT*> > >  TaskItemTransparencyVector;
typedef std::pair<std::tstring, std::map<UINT, UINT*> > TaskItemTransparencyPair;

class TaskItemTransparency {
	public:
		TaskItemTransparencyVector data;

		TaskItemTransparency(void) {  }
		TaskItemTransparencyVector::iterator find_ts(tstring key) {
			TaskItemTransparencyVector::iterator iter = data.begin();
			
			for(; iter != data.end(); iter++) {
				if(iter->first == key)
					return iter;
			}
			
			return this->end();
		}
		TaskItemTransparencyVector::iterator begin(void) {
			return data.begin();
		}
		TaskItemTransparencyVector::iterator end(void) {
			return data.end();
		}
		TaskItemTransparencyPair& operator[](const tstring &key) {
			TaskItemTransparencyVector::iterator found = find_ts(key);
			if (found == this->end()) {
				TaskItemTransparencyPair newdata;
				newdata.first = key;
				this->data.push_back(newdata);
				return this->data[this->data.size()-1];
			}
			return *found;
		}
};

class TransSettingParams {
public:
	TransSettingParams();
	virtual ~TransSettingParams();
	TaskItemTransparency * getTaskItemTransparency() const { return m_TaskItemTransparency; }
	void setTaskItemTransparency(TaskItemTransparency *trans) { m_TaskItemTransparency = trans; }

private:
	TaskItemTransparency * m_TaskItemTransparency;
};

HWND showTransSettingDialog(HWND hOwnerWindow, TransSettingParams &params);

#endif  // TRANSSETDLG_H_
