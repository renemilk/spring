/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "InMapDrawView.h"
#include "Rendering/Colors.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/GlobalRendering.h"

#include "Game/Camera.h"
#include "Game/InMapDraw.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/TeamHandler.h"

CInMapDrawView* inMapDrawerView = NULL;

CR_BIND(CInMapDrawView, );

CR_REG_METADATA(CInMapDrawView, (
	CR_RESERVED(4)
));



CInMapDrawView::CInMapDrawView()
{
	unsigned char tex[64][128][4];
	for (int y = 0; y < 64; y++) {
		for (int x = 0; x < 128; x++) {
			tex[y][x][0] = 0;
			tex[y][x][1] = 0;
			tex[y][x][2] = 0;
			tex[y][x][3] = 0;
		}
	}

	#define SMOOTHSTEP(x,y,a) (unsigned char)(x * (1.0f - a) + y * a)

	for (int y = 0; y < 64; y++) {
		// circular thingy
		for (int x = 0; x < 64; x++) {
			float dist = sqrt((float)(x - 32) * (x - 32) + (y - 32) * (y - 32));
			if (dist > 31.0f) {
				// do nothing - leave transparent
			} else if (dist > 30.0f) {
				// interpolate (outline -> nothing)
				float a = (dist - 30.0f);
				tex[y][x][3] = SMOOTHSTEP(255,   0, a);
			} else if (dist > 24.0f) {
				// black outline
				tex[y][x][3] = 255;
			} else if (dist > 23.0f) {
				// interpolate (inner -> outline)
				float a = (dist - 23.0f);
				tex[y][x][0] = SMOOTHSTEP(255,   0, a);
				tex[y][x][1] = SMOOTHSTEP(255,   0, a);
				tex[y][x][2] = SMOOTHSTEP(255,   0, a);
				tex[y][x][3] = SMOOTHSTEP(200, 255, a);
			} else {
				tex[y][x][0] = 255;
				tex[y][x][1] = 255;
				tex[y][x][2] = 255;
				tex[y][x][3] = 200;
			}
		}
	}
	for (int y = 0; y < 64; y++) {
		// linear falloff
		for (int x = 64; x < 128; x++) {
			float dist = abs(y - 32);
			if (dist > 31.0f) {
				// do nothing - leave transparent
			} else if (dist > 30.0f) {
				// interpolate (outline -> nothing)
				float a = (dist - 30.0f);
				tex[y][x][3] = SMOOTHSTEP(255,   0, a);
			} else if (dist > 24.0f) {
				// black outline
				tex[y][x][3] = 255;
			} else if (dist > 23.0f) {
				// interpolate (inner -> outline)
				float a = (dist - 23.0f);
				tex[y][x][0] = SMOOTHSTEP(255,   0, a);
				tex[y][x][1] = SMOOTHSTEP(255,   0, a);
				tex[y][x][2] = SMOOTHSTEP(255,   0, a);
				tex[y][x][3] = SMOOTHSTEP(200, 255, a);
			} else {
				tex[y][x][0] = 255;
				tex[y][x][1] = 255;
				tex[y][x][2] = 255;
				tex[y][x][3] = 200;
			}
		}
	}

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, 128, 64, GL_RGBA, GL_UNSIGNED_BYTE, tex[0]);
}


CInMapDrawView::~CInMapDrawView()
{
	glDeleteTextures(1, &texture);
}

static unsigned char DEFAULT_COLOR[4] = {0, 0, 0, 255};

static void SerializeColor(creg::ISerializer &s, unsigned char** color)
{
	if (s.IsWriting()) {
		char ColorType = 0;
		char ColorId = 0;
		if (!ColorType)
			for (int a = 0; a < teamHandler->ActiveTeams(); ++a)
				if (*color == teamHandler->Team(a)->color) {
					ColorType = 1;
					ColorId = a;
					break;
				}
		s.Serialize(&ColorId, sizeof(ColorId));
		s.Serialize(&ColorType, sizeof(ColorType));
	} else {
		char ColorType;
		char ColorId;
		s.Serialize(&ColorId, sizeof(ColorId));
		s.Serialize(&ColorType, sizeof(ColorType));
		switch (ColorType) {
			case 0:{
				*color = DEFAULT_COLOR;
				break;
			}
			case 1:{
				*color = teamHandler->Team(ColorId)->color;
				break;
			}
		}
	}
}

struct InMapDraw_QuadDrawer: public CReadMap::IQuadDrawer
{
	CVertexArray* va;
	CVertexArray* lineva;
	CInMapDraw* imd;
	std::vector<const CInMapDraw::MapPoint*>* visLabels;

	void DrawQuad(int x, int y);
};

void InMapDraw_QuadDrawer::DrawQuad(int x, int y)
{
	const CInMapDraw::DrawQuad* dq = imd->GetDrawQuad(x, y);

	va->EnlargeArrays(dq->points.size() * 12, 0, VA_SIZE_TC);
	//! draw point markers
	for (std::list<CInMapDraw::MapPoint>::const_iterator pi = dq->points.begin(); pi != dq->points.end(); ++pi) {
		if (pi->IsLocalPlayerAllowedToSee(imd)) {
			const float3 pos = pi->pos;
			const float3 dif = (pos - camera->pos).ANormalize();
			const float3 dir1 = (dif.cross(UpVector)).ANormalize();
			const float3 dir2 = (dif.cross(dir1));


			const unsigned char* color = pi->IsBySpectator() ? color4::white : teamHandler->Team(pi->GetTeamID())->color;
			const unsigned char col[4] = {
				color[0],
				color[1],
				color[2],
				200
			};

			const float size = 6.0f;
			const float3 pos1(pos.x,  pos.y  +   5.0f, pos.z);
			const float3 pos2(pos1.x, pos1.y + 100.0f, pos1.z);

			va->AddVertexQTC(pos1 - dir1 * size,               0.25f, 0, col);
			va->AddVertexQTC(pos1 + dir1 * size,               0.25f, 1, col);
			va->AddVertexQTC(pos1 + dir1 * size + dir2 * size, 0.00f, 1, col);
			va->AddVertexQTC(pos1 - dir1 * size + dir2 * size, 0.00f, 0, col);

			va->AddVertexQTC(pos1 - dir1 * size,               0.75f, 0, col);
			va->AddVertexQTC(pos1 + dir1 * size,               0.75f, 1, col);
			va->AddVertexQTC(pos2 + dir1 * size,               0.75f, 1, col);
			va->AddVertexQTC(pos2 - dir1 * size,               0.75f, 0, col);

			va->AddVertexQTC(pos2 - dir1 * size,               0.25f, 0, col);
			va->AddVertexQTC(pos2 + dir1 * size,               0.25f, 1, col);
			va->AddVertexQTC(pos2 + dir1 * size - dir2 * size, 0.00f, 1, col);
			va->AddVertexQTC(pos2 - dir1 * size - dir2 * size, 0.00f, 0, col);

			if (!pi->label.empty()) {
				visLabels->push_back(&*pi);
			}
		}
	}

	lineva->EnlargeArrays(dq->lines.size() * 2, 0, VA_SIZE_C);
	//! draw line markers
	for (std::list<CInMapDraw::MapLine>::const_iterator li = dq->lines.begin(); li != dq->lines.end(); ++li) {
		if (li->IsLocalPlayerAllowedToSee(imd)) {
			const unsigned char* color = li->IsBySpectator() ? color4::white : teamHandler->Team(li->GetTeamID())->color;
			lineva->AddVertexQC(li->pos - (li->pos - camera->pos).ANormalize() * 26, color);
			lineva->AddVertexQC(li->pos2 - (li->pos2 - camera->pos).ANormalize() * 26, color);
		}
	}
}



void CInMapDrawView::Draw()
{
	GML_STDMUTEX_LOCK(inmap); //! Draw

	CVertexArray* va = GetVertexArray();
	va->Initialize();
	CVertexArray* lineva = GetVertexArray();
	lineva->Initialize();

	InMapDraw_QuadDrawer drawer;
	drawer.imd = inMapDrawer;
	drawer.lineva = lineva;
	drawer.va = va;
	drawer.visLabels = &visibleLabels;

	glDepthMask(0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, texture);

	readmap->GridVisibility(camera, CInMapDraw::DRAW_QUAD_SIZE, 3000.0f, &drawer);

	glDisable(GL_TEXTURE_2D);
	glLineWidth(3.f);
	lineva->DrawArrayC(GL_LINES); //! draw lines

	// XXX hopeless drivers, retest in a year or so...
	// width greater than 2 causes GUI flicker on ATI hardware as of driver version 9.3
	// so redraw lines with width 1
	if (globalRendering->atiHacks) {
		glLineWidth(1.f);
		lineva->DrawArrayC(GL_LINES);
	}

	// draw points
	glLineWidth(1);
	glEnable(GL_TEXTURE_2D);
	va->DrawArrayTC(GL_QUADS); //! draw point markers 

	if (!visibleLabels.empty()) {
		font->SetColors(); //! default

		//! draw point labels
		for (std::vector<const CInMapDraw::MapPoint*>::const_iterator pi = visibleLabels.begin(); pi != visibleLabels.end(); ++pi) {
			float3 pos = (*pi)->pos;
			pos.y += 111.0f;

			const unsigned char* color = (*pi)->IsBySpectator() ? color4::white : teamHandler->Team((*pi)->GetTeamID())->color;
			font->SetTextColor(color[0]/255.0f, color[1]/255.0f, color[2]/255.0f, 1.0f); //FIXME (overload!)
			font->glWorldPrint(pos, 26.0f, (*pi)->label);
		}

		visibleLabels.clear();
	}

	glDepthMask(1);
}