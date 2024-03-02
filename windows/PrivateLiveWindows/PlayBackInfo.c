#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
//SDL
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include "tchar.h"
#include "aes.h"

#define ENABLE_COPY_WHILE_PLAY
#define	OUTBUFFER_SIZE		(1024*1024)
AES_KEY AESEnckey;
AES_KEY AESDeckey;


static void set_encrypt_pwd(unsigned char *pwd)
{
	unsigned char 			Key[16];

	memset(Key,0x00,16);
	strncpy((char *)Key, (char *)pwd, 15);

	AES_set_encrypt_key((const unsigned char *) Key, 128, &AESEnckey);	
	
}

static void set_decrypt_pwd(unsigned char *pwd)
{
	unsigned char 			Key[16];

	memset(Key,0x00,16);
	strncpy((char *)Key, (char *)pwd, 15);

	AES_set_decrypt_key((const unsigned char *) Key, 128, &AESDeckey);	
	
}

static void* encrypt(unsigned char *buffer, int buffer_size )
{
	int 					aligned_buffer_size;
	unsigned char			*temp_buffer;
	static unsigned char	inbuffer[16];
	static unsigned char	outbuffer[OUTBUFFER_SIZE];
	int						block_index;
	int						no_of_blocks;
	
	if((buffer_size&0x000F)!=0)
	{
		no_of_blocks = buffer_size>>4;
		
		temp_buffer = buffer ;
		
		for (block_index=0; block_index<no_of_blocks; block_index++)
		{
			AES_encrypt((const unsigned char *) &temp_buffer[block_index<<4], (unsigned char *) &outbuffer[block_index<<4], (const AES_KEY *) &AESEnckey);
		}

		//memset(inbuffer, 0x00, 16);

		memcpy( &outbuffer[block_index<<4], &temp_buffer[block_index<<4], buffer_size&0x000F );
		
		//AES_encrypt((const unsigned char *) inbuffer, (unsigned char *) &outbuffer[block_index<<4], (const AES_KEY *) &AESEnckey);
		
	}
	else
	{
		no_of_blocks = buffer_size>>4;
		
		temp_buffer = buffer ;
		
		for (block_index=0; block_index<no_of_blocks; block_index++)
		{
			AES_encrypt((const unsigned char *) &temp_buffer[block_index<<4], (unsigned char *)&outbuffer[block_index<<4], (const AES_KEY *) &AESEnckey);
		}
	}
	return outbuffer;
}

static void* decrypt(unsigned char *buffer, int buffer_size )
{
	int 					aligned_buffer_size;
	unsigned char			*temp_buffer;
	static unsigned char	inbuffer[16];
	static unsigned char	outbuffer[OUTBUFFER_SIZE];
	int 					block_index;
	int 					no_of_blocks;
	
	if((buffer_size&0x000F)!=0)
	{
		no_of_blocks = buffer_size>>4;
		
		temp_buffer = buffer ;
		
		for (block_index=0; block_index<no_of_blocks; block_index++)
		{
			AES_decrypt((const unsigned char *) &temp_buffer[block_index<<4], (unsigned char *) &outbuffer[block_index<<4], (const AES_KEY *) &AESDeckey);
		}

		//memset(inbuffer, 0x00, 16);

		memcpy( &outbuffer[block_index<<4], &temp_buffer[block_index<<4], buffer_size&0x000F );
		
		//AES_decrypt((const unsigned char *) &inbuffer, (unsigned char *) &outbuffer[block_index<<4], (const AES_KEY *) &AESDeckey);
		
	}
	else
	{
		no_of_blocks = buffer_size>>4;
		
		temp_buffer = buffer ;
		
		for (block_index=0; block_index<no_of_blocks; block_index++)
		{
			AES_decrypt((const unsigned char *) &temp_buffer[block_index<<4], (unsigned char *) &outbuffer[block_index<<4], (const AES_KEY *) &AESDeckey);
		}
	}
	return outbuffer;
}


int _tmain(int argc, _TCHAR* argv[])
{
	AVFormatContext	*pFormatCtx;
	int				i, videoindex,audioindex;
	AVCodecContext	*pCodecCtx_video;
	AVCodecContext	*pCodecCtx_audio;	
	AVCodec			*pCodec;
	//char* 			filepath="E:/movies/jigarthanda/samplemovie.mp4";
	char			filepath[100];
	char			outputfile[100];
	uint8_t 		*out_buffer;
	SDL_Surface 	*screen; 
	SDL_Rect 		rect;
	SDL_Overlay 	*bmp; 

#ifdef ENABLE_COPY_WHILE_PLAY
	AVFormatContext	*oc;
	AVStream 		*out_stream_video;
	AVStream 		*out_stream_audio;	
	AVStream 		*in_stream;
	int				index;

#endif /* ENABLE_COPY_WHILE_PLAY */
	void *temp_buffer;
	char			pin[10];
	//--------
	int ret, got_picture;
	int y_size;
	
	AVPacket *packet;
	struct SwsContext *img_convert_ctx;
#if 1
	printf ( "Note : Never use \\ in the path name. Instead always use /\n" );
	printf ( "\nEnter input file name  : " );
	scanf ( "%s", filepath );
	printf ( "\nName of encrypted file  : " );
	scanf ( "%s", outputfile );
	printf ( "\nEnter pin : \n" );
	scanf ( "%s", pin );
	set_encrypt_pwd(pin);
#else
	strcpy(filepath, "E:/working/player/decryptor/test/original_movie.mp4");
	strcpy(outputfile, "E:/working/player/decryptor/test/encrypted_movie.mp4");
	set_encrypt_pwd("aashi");
#endif
	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();
	
	if(avformat_open_input(&pFormatCtx,filepath,NULL,NULL)!=0){
		printf("Couldn't open input stream.\n");
		return -1;
	}
	
	if(avformat_find_stream_info(pFormatCtx,NULL)<0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}

	videoindex=-1;
	audioindex=-1;
	for(i=0; i<pFormatCtx->nb_streams; i++) 
	{
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
		{
			videoindex=i;
		}

		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO)
		{
			audioindex=i;
		}
	}
		
	if((videoindex==-1)||(audioindex==-1))
	{
		printf("Didn't find a video stream.\n");
		return -1;
	}
	pCodecCtx_video=pFormatCtx->streams[videoindex]->codec;
	pCodecCtx_audio=pFormatCtx->streams[audioindex]->codec;	

	avformat_alloc_output_context2( &oc, NULL, NULL, outputfile);

	out_stream_video = avformat_new_stream(oc, pCodecCtx_video->codec);

	ret = avcodec_copy_context(out_stream_video->codec, pCodecCtx_video);

	if (ret < 0) 
	{
		return -1;
	}

	out_stream_audio= avformat_new_stream(oc, pCodecCtx_audio->codec);

	ret = avcodec_copy_context(out_stream_audio->codec, pCodecCtx_audio);

	if (ret < 0) 
	{
		return -1;
	}

	out_stream_video->codec->pix_fmt = pCodecCtx_video->pix_fmt;
	out_stream_video->codec->codec_tag = 0;
	out_stream_video->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

	out_stream_audio->codec->codec_tag = 0;
	out_stream_audio->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

	ret = avio_open(&oc->pb, outputfile, AVIO_FLAG_WRITE);
	if (ret < 0) 
	{
		return -1;
	}

	ret = avformat_write_header(oc, &pFormatCtx->metadata);
	if (ret < 0) 
	{
		return -1;
	}



	packet=(AVPacket *)av_malloc(sizeof(AVPacket));
	y_size = pCodecCtx_video->width * pCodecCtx_video->height;
	av_new_packet(packet, y_size);

	printf("File Information---------------------\n");
	av_dump_format(pFormatCtx,0,filepath,0);
	printf("-------------------------------------------------\n");
	
	img_convert_ctx = sws_getContext(pCodecCtx_video->width, pCodecCtx_video->height, pCodecCtx_video->pix_fmt, pCodecCtx_video->width, pCodecCtx_video->height, PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL); 

	while(av_read_frame(pFormatCtx, packet)>=0)
	{
		if(packet->stream_index==videoindex)
		{
			in_stream  = pFormatCtx->streams[packet->stream_index];
			out_stream_video = oc->streams[packet->stream_index];
			
			/* copy packet */
			packet->pts = av_rescale_q_rnd(packet->pts, in_stream->time_base, out_stream_video->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
			packet->dts = av_rescale_q_rnd(packet->dts, in_stream->time_base, out_stream_video->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
			packet->duration = av_rescale_q(packet->duration, in_stream->time_base, out_stream_video->time_base);
			packet->pos = -1;
#if 0
			for(index = 0; index  < packet->size; index++)
			{
				unsigned char ch;
				unsigned char enc_ch;
				
				ch = packet->data[index];
				enc_ch = ((ch&0xF0)>>4)|((ch&0x0F)<<4);
				packet->data[index] = enc_ch;
			}
#else
			temp_buffer = encrypt(packet->data, packet->size);
			memcpy(packet->data,temp_buffer,packet->size);

#endif
			av_interleaved_write_frame(oc, packet);
		}
		if(packet->stream_index==audioindex)
		{

			in_stream  = pFormatCtx->streams[packet->stream_index];
			out_stream_audio = oc->streams[packet->stream_index];
			
			/* copy packet */
			packet->pts = av_rescale_q_rnd(packet->pts, in_stream->time_base, out_stream_audio->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
			packet->dts = av_rescale_q_rnd(packet->dts, in_stream->time_base, out_stream_audio->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
			packet->duration = av_rescale_q(packet->duration, in_stream->time_base, out_stream_audio->time_base);
			packet->pos = -1;

#if 0
			for(index = 0; index  < packet->size; index++)
			{
				unsigned char ch;
				unsigned char enc_ch;
				
				ch = packet->data[index];
				enc_ch = ((ch&0xF0)>>4)|((ch&0x0F)<<4);
				packet->data[index] = enc_ch;
			}
#else
			temp_buffer = encrypt(packet->data, packet->size);
			memcpy(packet->data,temp_buffer,packet->size);
#endif

			av_interleaved_write_frame(oc, packet);
		}		
		av_free_packet(packet);
	}

	av_write_trailer(oc);
	avio_close(oc->pb);
	avformat_free_context(oc);
	
	sws_freeContext(img_convert_ctx);


	avcodec_close(pCodecCtx_video);
	avformat_close_input(&pFormatCtx);

	printf ( "\nEncryption completed\n" );
	
}
