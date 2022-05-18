#ifndef NES_H
#define NES_H

//#define SECTION_FAST //.section emu.fast, "awx"
//#define SECTION_SLOW //.section emu.slow, "awx"
//#define SECTION_HEAD //.section emu.head, "awx"

#define ALIGN .align 4
#define SECTION_DATA .data
#define SECTION_TEXT .text

/*
 * 6502 のレジスタ
 *
 * A は常に 24 ビットシフトしておく
 * P は2つに分ける。
 * NZはほとんどの命令が変更するので，NZの元になる値を保存
 * 残りは他の場所に置く
 *
 * S は24ビットシフトしておく
 * これで余って下のビットに P の残りを置く(VBDI)
 * さらに余ってるところには割り込みに関するフラグを置く
 *
 * PC はROM内のアドレスに変換しておく
 * この場合は境界チェックをしたほうがいいのだが，簡単ではない
 * 本当の PC を得るためには現在のバンクの先頭アドレスを引けばよい
 *
 * I just love the above comments :)
 */

//				// offs in nes_registers
#define REG_A  r4		// 00
#define REG_X  r5		// 04
#define REG_Y  r6		// 08
#define REG_PC r7		// 0c
#define REG_S  r8		// 10
#define REG_P_REST r8
#define REG_NZ  r9		// 14
#define REG_ADDR r10		// 18
#define REG_CYCLE r11		// 1c [31:16] - fceu cycles, [15:8] - fceu irqhook cycles, [7:0] - fceu timestamp cycles
#define REG_OP_TABLE r12	// 20


/*
 * REG_P_REST には各種フラグを置く
 *
 * 0
 * FEDCBA98 76543210
 * |||||||| |||||||+---C キャリーまたはボローなし
 * |||||||| ||||||+---NMIの発生
 * |||||||| |||||+---I 割り込み許可
 * |||||||| ||||+---D 十進モード
 * |||||||| |||+---B
 * |||||||| ||+---IRQの発生
 * |||||||| |+---V オーバーフロー
 * |||||||| +---$2005/$2006 トグルフラグ
 * ||||||||
 * |||||||+---$2000[2] 32インクリメント
 * ||||||+---$2000[3] スプライトアドレス選択
 * |||||+---$2000[4] バックグラウンドアドレス選択
 * ||||+---$2000[5] スプライトサイズ
 * |||+---未使用
 * ||+---$2000[7] NMI許可
 * |+---$2001[3] バックグラウンドを描画
 * +---$2001[4] スプライトを描画
 *
 * 1
 * FEDCBA9876543210
 * |||||||||||||||+---$2002[4] VRAMに書き込み可？
 * ||||||||||||||+---$2002[5] スプライトが多すぎ
 * |||||||||||||+---$2002[6] スプライトヒット(実装できるのか？)
 * ||||||||||||+---$2002[7] VBL
 * |||||||||||+---DMC割り込み発生
 * ||||||||||+---VRAMを持っている
 * ||||||||++---未使用
 * ++++++++---S スタックポインタ
 *
 * note: fceu uses this differently
 * [7:0]   - flags (same as above)
 * [15:8]  - FCEU IRQ pending sources
 * [16]    - a flag which indicates that MapIRQHook is not null
 * [23:17] - unused
 * [31:24] - stack pointer
 */


/*
 * REG_P_REST で使うフラグ。基本的にPと同じ位置にある
 */
#define P_REST_V_FLAG 0x40
#define P_REST_B_FLAG 0x10
#define P_REST_D_FLAG 0x08
#define P_REST_I_FLAG 0x04
#define P_REST_C_FLAG 0x01
#define P_REST_FLAGS  0x5D

/*
 * 割り込みが発生するとセットされる
 */
// will be using FCE flags instead of this
// #define P_REST_INT_PENDING 0x20
// #define P_REST_NMI_PENDING 0x02

// #define ASSERT_NMI	orr	REG_P_REST, REG_P_REST, #P_REST_NMI_PENDING
// #define ASSERT_INT	orr	REG_P_REST, REG_P_REST, #P_REST_INT_PENDING


/*
 * 6502 の本当のフラグ
 */
#define P_N_FLAG 0x80
#define P_V_FLAG 0x40
#define P_R_FLAG 0x20
#define P_B_FLAG 0x10
#define P_D_FLAG 0x08
#define P_I_FLAG 0x04
#define P_Z_FLAG 0x02
#define P_C_FLAG 0x01

#define NMI_VECTOR   6
#define RESET_VECTOR 4
#define IRQ_VECTOR   2

#endif

