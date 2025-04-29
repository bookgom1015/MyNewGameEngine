#ifndef __LIGHTTYPE_H__
#define __LIGHTTYPE_H__

namespace Common {
	namespace Render {
		enum LightType {
			E_None = 0,
			E_Directional,
			E_Point,
			E_Spot,
			E_Tube,
			E_Rect,
			Count
		};
	}
}

#endif // __LIGHTTYPE_H__