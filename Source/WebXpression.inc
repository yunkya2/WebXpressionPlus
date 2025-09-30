*	WebXpression.inc
*		注：WebXpression.h と同内容にすること！

*	行管理テーブル
	.offset	0
ptr:		.ds.l	1	* テキストへのポインタ
start_dot:	.ds.w	1	* 左端から何ドット目から表示開始するか
org_line:	.ds.w	1	* 元の HTML の何行目だったか（エディタ起動用）
font_size:	.ds.b	1	* サイズ
font_type:	.ds.b	1	* =0 なら標準フォント
font_decoration:.ds.b	1	* 文字装飾
dammy:		.ds.b	1	* ダミー
size_of_line_ptr:


*	リンクテーブル構造体
	.offset	0
link_table_url:		.ds.l	1
link_table_in_cache:	.ds.b	1
link_table_dammy:	.ds.b	1
link_table_size:


*	イメージリスト構造体
	.offset	0
image_list_next_ptr:	.ds.l	1
image_list_before_ptr:	.ds.l	1
image_list_count:	.ds.w	1
image_list_x:		.ds.w	1
image_list_y		.ds.w	1
image_list_data:	.ds.l	1
image_list_url:		.ds.b	256
image_list_size:


*	イメージテーブル構造体
	.offset	0
image_table_image_list:	.ds.l	1
image_table_disp_x	.ds.w	1
image_table_in_cache:	.ds.b	1
image_table_dammy:	.ds.b	1
image_table_size:


*	Xpression 形式テキスト管理構造体
	.offset	0
xptext_filesize:	.ds.l	1
xptext_line_ptr:	.ds.l	1
xptext_text:		.ds.l	1
xptext_line:		.ds.l	1
xptext_current_line:	.ds.l	1
xptext_link_table:	.ds.l	1
xptext_link_table_max:	.ds.w	1
xptext_link_table_buffer:.ds.l	1
xptext_image_table:	.ds.l	1
xptext_image_table_max:	.ds.w	1
xptext_anchor_table:	.ds.l	1
xptext_anchor_table_max:	.ds.w	1
xptext_title:		.ds.b	65


*	イベント構造体
		.offset	0
event_type:	.ds.b	1	* イベントの種類
event_type2:	.ds.b	1	* イベントの種類その２
event_shift:	.ds.b	1	* = 非0: [SHIFT]キーが押されている
event_ctrl:	.ds.b	1	* = 非0: [CTRL]キーが押されている
mouse_pos:
mouse_x:	.ds.w	1	* マウスカーソルの座標
mouse_y:	.ds.w	1
mouse_button:	.ds.w	1
mouse_pat:	.ds.w	1
old_mouse_pos:
old_mouse_x:	.ds.w	1	* 前のマウスカーソルの座標
old_mouse_y:	.ds.w	1
old_mouse_pat:	.ds.w	1
link_num:	.ds.w	1
check_xptext:	.ds.l	1	* 表示中の xptext
keycode:	.ds.l	1	* キーコード
