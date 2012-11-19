package com.media.ffmpeg.android;

//~--- non-JDK imports --------------------------------------------------------

import android.content.Context;

import android.util.AttributeSet;
import android.util.Log;

import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

import android.widget.MediaController;
import android.widget.MediaController.MediaPlayerControl;

import com.media.ffmpeg.FFMpegPlayer;
import com.media.ffmpeg.FFMpegPlayer.OnCompletionListener;

//~--- JDK imports ------------------------------------------------------------

import java.io.IOException;

public class FFMpegMovieViewAndroid extends SurfaceView {
    private static final String TAG         = "FFMpegMovieViewAndroid";
    SurfaceHolder.Callback      mSHCallback = new SurfaceHolder.Callback() {
        public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
            startRenderVideo();
        }
        public void surfaceCreated(SurfaceHolder holder) {
            setDisplay(holder);
        }
        public void surfaceDestroyed(SurfaceHolder holder) {
            release();

            if (mMediaController.isShowing()) {
                mMediaController.hide();
            }
        }
    };
    MediaPlayerControl mMediaPlayerControl = new MediaPlayerControl() {
        public void start() {
            Log.d(TAG, "start() ");
            //mPlayer.resume();
            if (!mPlayer.isPlaying()) {
            	mPlayer.start();
            }
        }
        public void seekTo(int msec) {
            Log.d(TAG, "seek to " + msec);
            if (mPlayer != null) {
                mPlayer.seekTo(msec);
            }
        }
        public void pause() {
            Log.d(TAG, "pause() ");
            mPlayer.pause();
        }
        public boolean isPlaying() {
            return mPlayer.isPlaying();
        }
        public int getDuration() {
            return mPlayer.getDuration();
        }
        public int getCurrentPosition() {
            return mPlayer.getCurrentPosition();
        }
        public int getBufferPercentage() {

            // Log.d(TAG, "want buffer percentage");
            return 0;
        }
        @Override
        public boolean canPause() {

            // TODO Auto-generated method stub
            return true;
        }
        @Override
        public boolean canSeekBackward() {

            // TODO Auto-generated method stub
            return true;
        }
        @Override
        public boolean canSeekForward() {

            // TODO Auto-generated method stub
            return true;
        }
    };
    private FFMpegPlayer    mPlayer;
    private MediaController mMediaController;

    public FFMpegMovieViewAndroid(Context context) {
        super(context);
        initVideoView();
    }

    public FFMpegMovieViewAndroid(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
        initVideoView();
    }

    public FFMpegMovieViewAndroid(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        initVideoView();
    }

    private void initVideoView() {
        mPlayer = new FFMpegPlayer();

        /* set some callbacks to mPlayer */
        OnCompletionListener listener = new OnCompletionListener() {
            @Override
            public void onCompletion(FFMpegPlayer mp) {

                // TODO Auto-generated method stub
                Log.e(TAG, "xxx Playback End!");
            }
        };

        mPlayer.setOnCompletionListener(listener);

        SurfaceHolder surfHolder = getHolder();

        surfHolder.addCallback(mSHCallback);
    }

    private void attachMediaController() {
        mMediaController = new MediaController(getContext());

        View anchorView = (this.getParent() instanceof View)
                          ? (View) this.getParent()
                          : this;

        mMediaController.setMediaPlayer(mMediaPlayerControl);
        mMediaController.setAnchorView(anchorView);

        OnClickListener nextClickListener = new OnClickListener() {
            @Override
            public void onClick(View v) {
                // TODO Auto-generated method stub
                Log.d(TAG, "Next btn is clicked!");
            }
        };
        OnClickListener prevClickListener = new OnClickListener() {
            @Override
            public void onClick(View v) {
                // TODO Auto-generated method stub
                Log.d(TAG, "Pre btn is clicked!");
            }
        };

        mMediaController.setPrevNextListeners(nextClickListener, prevClickListener);
        mMediaController.setEnabled(true);
    }

    public void setVideoPath(String filePath) throws IllegalArgumentException, IllegalStateException, IOException {
        mPlayer.setDataSource(filePath);
    }

    /**
     * initzialize player
     */
    private void setDisplay(SurfaceHolder surfHolder) {
        try {
            mPlayer.setDisplay(surfHolder);
            mPlayer.prepare();
        } catch (IllegalStateException e) {
            Log.e(TAG, "Couldn't prepare player: " + e.getMessage());
        } catch (IOException e) {
            Log.e(TAG, "Couldn't prepare player: " + e.getMessage());
        }
    }

    private void startRenderVideo() {
        attachMediaController();
        mPlayer.start();
    }

    private void release() {
        Log.d(TAG, "releasing player");
        mPlayer.suspend();
        Log.d(TAG, "released");
    }

    public boolean onTouchEvent(android.view.MotionEvent event) {
        if (!mMediaController.isShowing()) {
            mMediaController.show(5000);
        }

        return true;
    }
}


//~ Formatted by Jindent --- http://www.jindent.com
