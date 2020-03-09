#ifndef __CFFMPEG_VIDEO_PLAYER_H__
#define __CFFMPEG_VIDEO_PLAYER_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include <vector>

USING_NS_CC;
using namespace ui;
using namespace std;

class AVPicture;
class AVFormatContext;
class AVCodecContext;
class AVCodec;
class AVPacket;
class AVFrame;
class SwsContext;

class CFFmpegVideoPlayer :
	public Sprite
{
public:
	CFFmpegVideoPlayer();
	virtual ~CFFmpegVideoPlayer();

	static CFFmpegVideoPlayer* create(const char * filename, int width, int height, bool bLoop = true);
	bool init(const char * filename, int width, int height, bool bLoop = true);

	void play();
	void stop();

private:
	void updateTimer(float dt);
	virtual void draw(Renderer *renderer, const Mat4 &transform, uint32_t flags) override;
private:
	bool m_bLoop;
	Size m_videoSize;
	Size m_videoDisplaySize;
	float m_fDtPlay;

	AVPicture * m_pAvpicture;
	AVFormatContext * m_pFormatCtx;
	AVCodecContext * m_pCodecCtx;
	const AVCodec * m_codec;
	AVFrame * m_pFrame;
	SwsContext * m_img_convert_ctx;


	int videoIndex;
};

int register_all_CFFmpegVideoPlayer(lua_State* tolua_S);

#endif // __CFFMPEG_VIDEO_PLAYER_H__
