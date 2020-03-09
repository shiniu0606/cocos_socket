#include "FFmpegVideoPlayer.h"
#include "tolua_fix.h"
#include "LuaBasicConversions.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "tolua++.h"
#ifdef __cplusplus
}
#endif

#ifndef INT64_C 
#define INT64_C(c) (c ## LL) 
#define UINT64_C(c) (c ## ULL) 
#endif

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/avutil.h"
#include "libavfilter/avfilter.h"
}

CFFmpegVideoPlayer::CFFmpegVideoPlayer():
	m_bLoop(true),
	m_pFormatCtx(nullptr),
	m_pCodecCtx(nullptr),
	m_codec(nullptr),
	m_pFrame(nullptr),
	m_img_convert_ctx(nullptr),
	m_pAvpicture(nullptr),
	m_fDtPlay(1.0/60)
{
}

CFFmpegVideoPlayer::~CFFmpegVideoPlayer()
{
	stop();
	if (m_pFrame != nullptr)
		av_frame_free(&m_pFrame);
	if (m_img_convert_ctx != nullptr)
		sws_freeContext(m_img_convert_ctx);
	if (m_pAvpicture != nullptr) {
		avpicture_free(m_pAvpicture);
		delete m_pAvpicture;
	}
	if (m_pFormatCtx != nullptr)
		avformat_free_context(m_pFormatCtx);
}

CFFmpegVideoPlayer* CFFmpegVideoPlayer::create(const char * filename, int width, int height, bool bLoop)
{
	auto player = new (std::nothrow) CFFmpegVideoPlayer();
	if (player != nullptr && player->init(filename, width, height, bLoop)) {
		player->autorelease();
		return player;
	}
	CC_SAFE_DELETE(player);
	return nullptr;
}

bool CFFmpegVideoPlayer::init(const char * filename, int width, int height, bool bLoop)
{
	auto fileUtils = FileUtils::getInstance();
	auto fullpathname = fileUtils->fullPathForFilename(filename);
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
	fullpathname = FileUtils::getInstance()->getWritablePath() + filename;
	if (!fileUtils->isFileExist(fullpathname)) {
		auto fullPath = fileUtils->fullPathForFilename(filename);
		auto fileData = fileUtils->getDataFromFile(fullPath);
		fileUtils->writeDataToFile(fileData, fullpathname);
	}
#endif
	 
	m_videoDisplaySize.width = width;
	m_videoDisplaySize.height = height;

	m_bLoop = bLoop; 

	av_register_all();
	avcodec_register_all();
	m_pFormatCtx = avformat_alloc_context();
	auto ret = avformat_open_input(&m_pFormatCtx, fullpathname.c_str(), nullptr, nullptr);
	if (ret != 0) {
		char buff[1280];
		av_strerror(ret, buff, 1280);
		return false;
	}
	if (avformat_find_stream_info(m_pFormatCtx, nullptr) < 0) {
		return false;
	}
	videoIndex = -1;
	for (int i = 0; i < m_pFormatCtx->nb_streams; i++) {
		if (m_pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoIndex = i;
			break;
		}
	}
	if (videoIndex == -1) {
		return false;
	}
	m_pCodecCtx = m_pFormatCtx->streams[videoIndex]->codec;
	if (!m_pCodecCtx) {
		return false;
	}
	m_codec = avcodec_find_decoder(m_pCodecCtx->codec_id);
	if (!m_codec) {
		fprintf(stderr, "Codec not found\n");
		return false;
	}
	/* open it */
	if (avcodec_open2(m_pCodecCtx, m_codec, NULL) < 0) {
		fprintf(stderr, "Could not open m_codec\n");
		return false;
	}
	m_pFrame = av_frame_alloc();
	if (!m_pFrame) {
		fprintf(stderr, "Could not allocate video frame\n");
		return false;
	}
	auto W = m_videoSize.width = m_pCodecCtx->width;
	auto H = m_videoSize.height = m_pCodecCtx->height;

	setScaleX(m_videoDisplaySize.width / W);
	setScaleY(m_videoDisplaySize.height / H);

	m_img_convert_ctx = sws_getContext(m_pCodecCtx->width,
		m_pCodecCtx->height,
		m_pCodecCtx->pix_fmt,
		W,
		H,
		AV_PIX_FMT_RGB24,
		SWS_FAST_BILINEAR, NULL, NULL, NULL);

	AVRational rational = m_pFormatCtx->streams[videoIndex]->r_frame_rate;
	m_fDtPlay = 1.0 * rational.den / rational.num;

	m_pAvpicture = new AVPicture();
	avpicture_alloc(m_pAvpicture, AV_PIX_FMT_RGB24, W, H);

	memset(m_pAvpicture->data[videoIndex], 0, m_pAvpicture->linesize[videoIndex] * H);

	CCTexture2D *texture = new CCTexture2D();
	texture->initWithData(m_pAvpicture->data[videoIndex], m_pAvpicture->linesize[videoIndex] * H, Texture2D::PixelFormat::RGB888, W, H, CCSize(W, H));
	initWithTexture(texture);

	this->setContentSize(CCSize(W, H));
	
	auto listener = EventListenerTouchOneByOne::create();
	listener->onTouchBegan = [](Touch* t, Event*e)->bool {return true; };
	listener->setSwallowTouches(true);

	Director::getInstance()->getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, this);

	updateTimer(0.0f);

	return true;
}

void CFFmpegVideoPlayer::play() 
{
	this->schedule(schedule_selector(CFFmpegVideoPlayer::updateTimer), m_fDtPlay);
}

void CFFmpegVideoPlayer::stop()
{
	this->unschedule(schedule_selector(CFFmpegVideoPlayer::updateTimer));
}

void CFFmpegVideoPlayer::updateTimer(float dt)
{
	int w = m_videoSize.width;
	int h = m_videoSize.height;

	AVPacket pkt;
	int got_picture = -1;
	while (av_read_frame(m_pFormatCtx, &pkt) >= 0) {
		if (pkt.stream_index == videoIndex) {
			avcodec_decode_video2(m_pCodecCtx, m_pFrame, &got_picture, &pkt);
		}
		if (got_picture) {
			sws_scale(m_img_convert_ctx, m_pFrame->data, m_pFrame->linesize,
				0, m_pCodecCtx->height,
				m_pAvpicture->data, m_pAvpicture->linesize);
			break;
		} 
		av_free_packet(&pkt);
	}
	av_free_packet(&pkt);
	if (got_picture <= 0) {
		if (m_bLoop)
			av_seek_frame(m_pFormatCtx, videoIndex, 0, AVSEEK_FLAG_BACKWARD);
		else {
			stop();
		}
		return;
	}
}

void CFFmpegVideoPlayer::draw(Renderer *renderer, const Mat4 &transform, uint32_t flags)
{
	Sprite::draw(renderer, transform, flags);

	CC_PROFILER_START_CATEGORY(kCCProfilerCategorySprite, "CCSprite - draw");

	CC_NODE_DRAW_SETUP();

	ccGLBlendFunc(_blendFunc.src, _blendFunc.dst);

	ccGLBindTexture2D(_texture->getName());
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_videoSize.width, m_videoSize.height, 0, GL_RGB, GL_UNSIGNED_BYTE, m_pAvpicture->data[videoIndex]);

	//  
	// Attributes  
	//  

	ccGLEnableVertexAttribs(kCCVertexAttribFlag_PosColorTex);

#define kQuadSize sizeof(_quad.bl)  
	long offset = (long)&_quad;

	// vertex  
	int diff = offsetof(ccV3F_C4B_T2F, vertices);
	glVertexAttribPointer(kCCVertexAttrib_Position, 3, GL_FLOAT, GL_FALSE, kQuadSize, (void*)(offset + diff));

	// texCoods  
	diff = offsetof(ccV3F_C4B_T2F, texCoords);
	glVertexAttribPointer(kCCVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE, kQuadSize, (void*)(offset + diff));

	// color  
	diff = offsetof(ccV3F_C4B_T2F, colors);
	glVertexAttribPointer(kCCVertexAttrib_Color, 4, GL_UNSIGNED_BYTE, GL_TRUE, kQuadSize, (void*)(offset + diff));

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	CHECK_GL_ERROR_DEBUG();

	CC_INCREMENT_GL_DRAWS(1);

	CC_PROFILER_STOP_CATEGORY(kCCProfilerCategorySprite, "CCSprite - draw");
}

// ==================================================================


int lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_init(lua_State* tolua_S)
{
	int argc = 0;
	CFFmpegVideoPlayer* cobj = nullptr;
	bool ok = true;

#if COCOS2D_DEBUG >= 1
	tolua_Error tolua_err;
#endif


#if COCOS2D_DEBUG >= 1
	if (!tolua_isusertype(tolua_S, 1, "CFFmpegVideoPlayer", 0, &tolua_err)) goto tolua_lerror;
#endif

	cobj = (CFFmpegVideoPlayer*)tolua_tousertype(tolua_S, 1, 0);

#if COCOS2D_DEBUG >= 1
	if (!cobj)
	{
		tolua_error(tolua_S, "invalid 'cobj' in function 'lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_init'", nullptr);
		return 0;
	}
#endif

	argc = lua_gettop(tolua_S) - 1;
	if (argc == 3)
	{
		const char* arg0;
		int arg1;
		int arg2;

		std::string arg0_tmp; ok &= luaval_to_std_string(tolua_S, 2, &arg0_tmp, "CFFmpegVideoPlayer:init"); arg0 = arg0_tmp.c_str();

		ok &= luaval_to_int32(tolua_S, 3, (int *)&arg1, "CFFmpegVideoPlayer:init");

		ok &= luaval_to_int32(tolua_S, 4, (int *)&arg2, "CFFmpegVideoPlayer:init");
		if (!ok)
		{
			tolua_error(tolua_S, "invalid arguments in function 'lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_init'", nullptr);
			return 0;
		}
		bool ret = cobj->init(arg0, arg1, arg2);
		tolua_pushboolean(tolua_S, (bool)ret);
		return 1;
	}
	if (argc == 4)
	{
		const char* arg0;
		int arg1;
		int arg2;
		bool arg3;

		std::string arg0_tmp; ok &= luaval_to_std_string(tolua_S, 2, &arg0_tmp, "CFFmpegVideoPlayer:init"); arg0 = arg0_tmp.c_str();

		ok &= luaval_to_int32(tolua_S, 3, (int *)&arg1, "CFFmpegVideoPlayer:init");

		ok &= luaval_to_int32(tolua_S, 4, (int *)&arg2, "CFFmpegVideoPlayer:init");

		ok &= luaval_to_boolean(tolua_S, 5, &arg3, "CFFmpegVideoPlayer:init");
		if (!ok)
		{
			tolua_error(tolua_S, "invalid arguments in function 'lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_init'", nullptr);
			return 0;
		}
		bool ret = cobj->init(arg0, arg1, arg2, arg3);
		tolua_pushboolean(tolua_S, (bool)ret);
		return 1;
	}
	luaL_error(tolua_S, "%s has wrong number of arguments: %d, was expecting %d \n", "CFFmpegVideoPlayer:init", argc, 3);
	return 0;

#if COCOS2D_DEBUG >= 1
	tolua_lerror:
				tolua_error(tolua_S, "#ferror in function 'lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_init'.", &tolua_err);
#endif

				return 0;
}
int lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_stop(lua_State* tolua_S)
{
	int argc = 0;
	CFFmpegVideoPlayer* cobj = nullptr;
	bool ok = true;

#if COCOS2D_DEBUG >= 1
	tolua_Error tolua_err;
#endif


#if COCOS2D_DEBUG >= 1
	if (!tolua_isusertype(tolua_S, 1, "CFFmpegVideoPlayer", 0, &tolua_err)) goto tolua_lerror;
#endif

	cobj = (CFFmpegVideoPlayer*)tolua_tousertype(tolua_S, 1, 0);

#if COCOS2D_DEBUG >= 1
	if (!cobj)
	{
		tolua_error(tolua_S, "invalid 'cobj' in function 'lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_stop'", nullptr);
		return 0;
	}
#endif

	argc = lua_gettop(tolua_S) - 1;
	if (argc == 0)
	{
		if (!ok)
		{
			tolua_error(tolua_S, "invalid arguments in function 'lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_stop'", nullptr);
			return 0;
		}
		cobj->stop();
		lua_settop(tolua_S, 1);
		return 1;
	}
	luaL_error(tolua_S, "%s has wrong number of arguments: %d, was expecting %d \n", "CFFmpegVideoPlayer:stop", argc, 0);
	return 0;

#if COCOS2D_DEBUG >= 1
	tolua_lerror:
				tolua_error(tolua_S, "#ferror in function 'lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_stop'.", &tolua_err);
#endif

				return 0;
}
int lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_play(lua_State* tolua_S)
{
	int argc = 0;
	CFFmpegVideoPlayer* cobj = nullptr;
	bool ok = true;

#if COCOS2D_DEBUG >= 1
	tolua_Error tolua_err;
#endif


#if COCOS2D_DEBUG >= 1
	if (!tolua_isusertype(tolua_S, 1, "CFFmpegVideoPlayer", 0, &tolua_err)) goto tolua_lerror;
#endif

	cobj = (CFFmpegVideoPlayer*)tolua_tousertype(tolua_S, 1, 0);

#if COCOS2D_DEBUG >= 1
	if (!cobj)
	{
		tolua_error(tolua_S, "invalid 'cobj' in function 'lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_play'", nullptr);
		return 0;
	}
#endif

	argc = lua_gettop(tolua_S) - 1;
	if (argc == 0)
	{
		if (!ok)
		{
			tolua_error(tolua_S, "invalid arguments in function 'lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_play'", nullptr);
			return 0;
		}
		cobj->play();
		lua_settop(tolua_S, 1);
		return 1;
	}
	luaL_error(tolua_S, "%s has wrong number of arguments: %d, was expecting %d \n", "CFFmpegVideoPlayer:play", argc, 0);
	return 0;

#if COCOS2D_DEBUG >= 1
	tolua_lerror:
				tolua_error(tolua_S, "#ferror in function 'lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_play'.", &tolua_err);
#endif

				return 0;
}
int lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_create(lua_State* tolua_S)
{
	int argc = 0;
	bool ok = true;

#if COCOS2D_DEBUG >= 1
	tolua_Error tolua_err;
#endif

#if COCOS2D_DEBUG >= 1
	if (!tolua_isusertable(tolua_S, 1, "CFFmpegVideoPlayer", 0, &tolua_err)) goto tolua_lerror;
#endif

	argc = lua_gettop(tolua_S) - 1;

	if (argc == 3)
	{
		const char* arg0;
		int arg1;
		int arg2;
		std::string arg0_tmp; ok &= luaval_to_std_string(tolua_S, 2, &arg0_tmp, "CFFmpegVideoPlayer:create"); arg0 = arg0_tmp.c_str();
		ok &= luaval_to_int32(tolua_S, 3, (int *)&arg1, "CFFmpegVideoPlayer:create");
		ok &= luaval_to_int32(tolua_S, 4, (int *)&arg2, "CFFmpegVideoPlayer:create");
		if (!ok)
		{
			tolua_error(tolua_S, "invalid arguments in function 'lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_create'", nullptr);
			return 0;
		}
		CFFmpegVideoPlayer* ret = CFFmpegVideoPlayer::create(arg0, arg1, arg2);
		object_to_luaval<CFFmpegVideoPlayer>(tolua_S, "CFFmpegVideoPlayer", (CFFmpegVideoPlayer*)ret);
		return 1;
	}
	if (argc == 4)
	{
		const char* arg0;
		int arg1;
		int arg2;
		bool arg3;
		std::string arg0_tmp; ok &= luaval_to_std_string(tolua_S, 2, &arg0_tmp, "CFFmpegVideoPlayer:create"); arg0 = arg0_tmp.c_str();
		ok &= luaval_to_int32(tolua_S, 3, (int *)&arg1, "CFFmpegVideoPlayer:create");
		ok &= luaval_to_int32(tolua_S, 4, (int *)&arg2, "CFFmpegVideoPlayer:create");
		ok &= luaval_to_boolean(tolua_S, 5, &arg3, "CFFmpegVideoPlayer:create");
		if (!ok)
		{
			tolua_error(tolua_S, "invalid arguments in function 'lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_create'", nullptr);
			return 0;
		}
		CFFmpegVideoPlayer* ret = CFFmpegVideoPlayer::create(arg0, arg1, arg2, arg3);
		object_to_luaval<CFFmpegVideoPlayer>(tolua_S, "CFFmpegVideoPlayer", (CFFmpegVideoPlayer*)ret);
		return 1;
	}
	luaL_error(tolua_S, "%s has wrong number of arguments: %d, was expecting %d\n ", "CFFmpegVideoPlayer:create", argc, 3);
	return 0;
#if COCOS2D_DEBUG >= 1
	tolua_lerror:
				tolua_error(tolua_S, "#ferror in function 'lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_create'.", &tolua_err);
#endif
				return 0;
}
int lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_constructor(lua_State* tolua_S)
{
	int argc = 0;
	CFFmpegVideoPlayer* cobj = nullptr;
	bool ok = true;

#if COCOS2D_DEBUG >= 1
	tolua_Error tolua_err;
#endif



	argc = lua_gettop(tolua_S) - 1;
	if (argc == 0)
	{
		if (!ok)
		{
			tolua_error(tolua_S, "invalid arguments in function 'lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_constructor'", nullptr);
			return 0;
		}
		cobj = new CFFmpegVideoPlayer();
		cobj->autorelease();
		int ID = (int)cobj->_ID;
		int* luaID = &cobj->_luaID;
		toluafix_pushusertype_ccobject(tolua_S, ID, luaID, (void*)cobj, "CFFmpegVideoPlayer");
		return 1;
	}
	luaL_error(tolua_S, "%s has wrong number of arguments: %d, was expecting %d \n", "CFFmpegVideoPlayer:CFFmpegVideoPlayer", argc, 0);
	return 0;

#if COCOS2D_DEBUG >= 1
	tolua_error(tolua_S, "#ferror in function 'lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_constructor'.", &tolua_err);
#endif

	return 0;
}

static int lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_finalize(lua_State* tolua_S)
{
	printf("luabindings: finalizing LUA object (CFFmpegVideoPlayer)");
	return 0;
}

int lua_register_CFFmpegVideoPlayer_CFFmpegVideoPlayer(lua_State* tolua_S)
{
	tolua_usertype(tolua_S, "CFFmpegVideoPlayer");
	tolua_cclass(tolua_S, "CFFmpegVideoPlayer", "CFFmpegVideoPlayer", "cc.Sprite", nullptr);

	tolua_beginmodule(tolua_S, "CFFmpegVideoPlayer");
	tolua_function(tolua_S, "new", lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_constructor);
	tolua_function(tolua_S, "init", lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_init);
	tolua_function(tolua_S, "stop", lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_stop);
	tolua_function(tolua_S, "play", lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_play);
	tolua_function(tolua_S, "create", lua_CFFmpegVideoPlayer_CFFmpegVideoPlayer_create);
	tolua_endmodule(tolua_S);
	std::string typeName = typeid(CFFmpegVideoPlayer).name();
	g_luaType[typeName] = "CFFmpegVideoPlayer";
	g_typeCast["CFFmpegVideoPlayer"] = "CFFmpegVideoPlayer";
	return 1;
}
TOLUA_API int register_all_CFFmpegVideoPlayer(lua_State* tolua_S)
{
	tolua_open(tolua_S);

	tolua_module(tolua_S, nullptr, 0);
	tolua_beginmodule(tolua_S, nullptr);

	lua_register_CFFmpegVideoPlayer_CFFmpegVideoPlayer(tolua_S);

	tolua_endmodule(tolua_S);
	return 1;
}