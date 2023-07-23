#include <stdio.h>

#include <opencv2/opencv.hpp>

#ifdef __cplusplus
	extern "C" {
#endif
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
#ifdef __cplusplus
	}
#endif

// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
	#define av_frame_alloc avcodec_alloc_frame
	#define av_frame_free avcodec_free_frame
#endif

int main(int argc, char *argv[])
{
	// Initalizing these to NULL prevents segfaults!
	AVFormatContext*	pFormatCtx = NULL;
	int					videoStream;
	AVCodecContext*		pCodecCtxOrig = NULL;
	AVCodecContext*		pCodecCtx = NULL;
	AVCodec*			pCodec = NULL;
	AVFrame*			pFrame = NULL;
	AVPacket			packet;
	int					frameFinished;
	struct SwsContext*	sws_ctx = NULL;
	
	if (argc < 2)
	{
		printf("Please provide a movie file\n");
		return -1;
	}
	// Register all formats and codecs
	av_register_all();

	// Open video file
	if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0)
	{
		return -1; // Couldn't open file
	}
	
	// Retrieve stream information
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		return -1; // Couldn't find stream information
	}
	
	// Dump information about file onto standard error
	av_dump_format(pFormatCtx, 0, argv[1], 0);

	// Find the first video stream
	videoStream = -1;
	for (int i = 0; i < pFormatCtx->nb_streams; ++i)
	{
		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
	  		videoStream = i;
	  		break;
		}
	}
	if (videoStream == -1)
	{
		return -1; // Didn't find a video stream
	}

	// Get a pointer to the codec context for the video stream
	pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;
	// Find the decoder for the video stream
	pCodec = avcodec_find_decoder(pCodecCtxOrig->codec_id);
	if (pCodec == NULL)
	{
		fprintf(stderr, "Unsupported codec!\n");
		return -1; // Codec not found
	}
	// Copy context
	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0)
	{
		fprintf(stderr, "Couldn't copy codec context");
		return -1; // Error copying codec context
	}

	// Open codec
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		return -1; // Could not open codec
	}
	
	// Allocate video frame
	pFrame = av_frame_alloc();
	
	while(av_read_frame(pFormatCtx, &packet) >= 0)
	{
        if (packet.stream_index ==	videoStream)
        {
        	// Decode video frame
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
		
			// Did we get a video frame?
			if(frameFinished)
			{
				cv::Mat cvFrame;
				AVFrame dst;
				int w = pFrame->width;
				int h = pFrame->height;
				cvFrame = cv::Mat(h, w, CV_8UC3);
				dst.data[0] = (uint8_t*)cvFrame.data;
				avpicture_fill((AVPicture*)&dst, dst.data[0], AV_PIX_FMT_BGR24, w, h);

				AVPixelFormat src_pixfmt = (AVPixelFormat)pFrame->format;
				AVPixelFormat dst_pixfmt = AV_PIX_FMT_BGR24;
				sws_ctx = sws_getContext(w, h, src_pixfmt, w, h, dst_pixfmt,
						SWS_FAST_BILINEAR, NULL, NULL, NULL);

				if(sws_ctx == NULL)
				{
					fprintf(stderr, "Cannot initialize the conversion context!\n");
					exit(1);
				}

				sws_scale(sws_ctx, pFrame->data, pFrame->linesize, 0, h,
						dst.data, dst.linesize);
			
				cv::imshow("asdf", cvFrame);
			}
        }
        
        // Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
		
        if (cv::waitKey(2) == 27)
        {
            break;
        }
    }
	
	// Free the frame
	av_frame_free(&pFrame);

	// Close the codecs
	avcodec_close(pCodecCtx);
	avcodec_close(pCodecCtxOrig);

	// Close the video file
	avformat_close_input(&pFormatCtx);

	return 0;
}

