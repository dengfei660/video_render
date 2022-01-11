# video_render
video frame display lib intergrated wayland protocol and others compositors,

start api sequence:
1.render_open
2.render_set_callback
3.render_set_user_data
4.render_connect
5.render_set  //set KEY_FRAME_SIZE,KEY_MEDIASYNC_INSTANCE_ID,KEY_VIDEO_FORMAT

while do render_display_frame

if display frame end
api sequence
1.render_disconnect
2.render_close
