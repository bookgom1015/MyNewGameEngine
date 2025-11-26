#ifndef __TONEMAPPERTYPE_H__
#define __TONEMAPPERTYPE_H__

namespace Common {
	namespace Render {
		enum TonemapperType {
			E_Exponential = 0,
			E_Reinhard,
			E_ReinhardExt,
			E_Uncharted2,
			E_ACES,
			E_Log
		};
	}
}

#endif // __TONEMAPPERTYPE_H__