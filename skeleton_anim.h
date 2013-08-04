/* Residual - A 3D game interpreter
 *
 * Residual is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the AUTHORS
 * file distributed with this source distribution.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 * $URL$
 * $Id$
 *
 */

#ifndef STARK_SKELETON_ANIM_H
#define STARK_SKELETON_ANIM_H

#include "math/vector3d.h"
#include "engines/stark/gfx/coordinate.h"
#include "common/array.h"

namespace Common {
	class ReadStream;
}

namespace Stark {

class AnimKey {
public:
	uint32 _time;
	Math::Vector3d _rot;
	float _rotW;
	Math::Vector3d _pos;
};

class AnimNode {
public:
	~AnimNode() {
		for (Common::Array<AnimKey *>::iterator it = _keys.begin(); it != _keys.end(); ++it)
			delete *it;
	}

	uint32 _bone;
	Common::Array<AnimKey *> _keys;
};

/**
 * Data structure responsible for skeletal animation of an actor object.
 */
class SkeletonAnim {
public:
	SkeletonAnim();
	~SkeletonAnim();

	bool createFromStream(Common::ReadStream *stream);

	/**
	 * Get the interpolated bone coordinate for a given bone at a given animation timestamp
	 */
	Coordinate getCoordForBone(uint32 time, int boneIdx);

	/**
	 * Get total animation length (in ms)
	 */
	uint32 getLength() const { return _time; }

private:
	uint32 _id, _ver, _u1, _u2, _time;

	Common::Array<AnimNode *> _anims;
};

} // End of namespace Stark

#endif // STARK_SKELETON_ANIM_H
