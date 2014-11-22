/* 
   Source   : philosopher/ph64.v
   Synopsis : Dining philosopher problem with 64 philosophers.
   Authors  : Fabio Somenzi, Colorado. Rewritten by Cheng-Yin Wu, NTUGIEE
   Date     : 2013/02/20
   Version  : 1.0
*/

`include "philosopher.v"

module ph64(clk, reset, r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23, r24, r25, r26, r27, r28, r29, r30, r31, r32, r33, r34, r35, r36, r37, r38, r39, r40, r41, r42, r43, r44, r45, r46, r47, r48, r49, r50, r51, r52, r53, r54, r55, r56, r57, r58, r59, r60, r61, r62, r63, safe, live);
input clk, reset;
input r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23, r24, r25, r26, r27, r28, r29, r30, r31, r32, r33, r34, r35, r36, r37, r38, r39, r40, r41, r42, r43, r44, r45, r46, r47, r48, r49, r50, r51, r52, r53, r54, r55, r56, r57, r58, r59, r60, r61, r62, r63;
output safe, live;

wire [1:0] s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15, s16, s17, s18, s19, s20, s21, s22, s23, s24, s25, s26, s27, s28, s29, s30, s31, s32, s33, s34, s35, s36, s37, s38, s39, s40, s41, s42, s43, s44, s45, s46, s47, s48, s49, s50, s51, s52, s53, s54, s55, s56, s57, s58, s59, s60, s61, s62, s63;

assign safe = 
   ((s0 == `EATING) & (s1 == `EATING)) |
   ((s1 == `EATING) & (s2 == `EATING)) |
   ((s2 == `EATING) & (s3 == `EATING)) |
   ((s3 == `EATING) & (s4 == `EATING)) |
   ((s4 == `EATING) & (s5 == `EATING)) |
   ((s5 == `EATING) & (s6 == `EATING)) |
   ((s6 == `EATING) & (s7 == `EATING)) |
   ((s7 == `EATING) & (s8 == `EATING)) |
   ((s8 == `EATING) & (s9 == `EATING)) |
   ((s9 == `EATING) & (s10 == `EATING)) |
   ((s10 == `EATING) & (s11 == `EATING)) |
   ((s11 == `EATING) & (s12 == `EATING)) |
   ((s12 == `EATING) & (s13 == `EATING)) |
   ((s13 == `EATING) & (s14 == `EATING)) |
   ((s14 == `EATING) & (s15 == `EATING)) |
   ((s15 == `EATING) & (s16 == `EATING)) |
   ((s16 == `EATING) & (s17 == `EATING)) |
   ((s17 == `EATING) & (s18 == `EATING)) |
   ((s18 == `EATING) & (s19 == `EATING)) |
   ((s19 == `EATING) & (s20 == `EATING)) |
   ((s20 == `EATING) & (s21 == `EATING)) |
   ((s21 == `EATING) & (s22 == `EATING)) |
   ((s22 == `EATING) & (s23 == `EATING)) |
   ((s23 == `EATING) & (s24 == `EATING)) |
   ((s24 == `EATING) & (s25 == `EATING)) |
   ((s25 == `EATING) & (s26 == `EATING)) |
   ((s26 == `EATING) & (s27 == `EATING)) |
   ((s27 == `EATING) & (s28 == `EATING)) |
   ((s28 == `EATING) & (s29 == `EATING)) |
   ((s29 == `EATING) & (s30 == `EATING)) |
   ((s30 == `EATING) & (s31 == `EATING)) |
   ((s31 == `EATING) & (s32 == `EATING)) |
   ((s32 == `EATING) & (s33 == `EATING)) |
   ((s33 == `EATING) & (s34 == `EATING)) |
   ((s34 == `EATING) & (s35 == `EATING)) |
   ((s35 == `EATING) & (s36 == `EATING)) |
   ((s36 == `EATING) & (s37 == `EATING)) |
   ((s37 == `EATING) & (s38 == `EATING)) |
   ((s38 == `EATING) & (s39 == `EATING)) |
   ((s39 == `EATING) & (s40 == `EATING)) |
   ((s40 == `EATING) & (s41 == `EATING)) |
   ((s41 == `EATING) & (s42 == `EATING)) |
   ((s42 == `EATING) & (s43 == `EATING)) |
   ((s43 == `EATING) & (s44 == `EATING)) |
   ((s44 == `EATING) & (s45 == `EATING)) |
   ((s45 == `EATING) & (s46 == `EATING)) |
   ((s46 == `EATING) & (s47 == `EATING)) |
   ((s47 == `EATING) & (s48 == `EATING)) |
   ((s48 == `EATING) & (s49 == `EATING)) |
   ((s49 == `EATING) & (s50 == `EATING)) |
   ((s50 == `EATING) & (s51 == `EATING)) |
   ((s51 == `EATING) & (s52 == `EATING)) |
   ((s52 == `EATING) & (s53 == `EATING)) |
   ((s53 == `EATING) & (s54 == `EATING)) |
   ((s54 == `EATING) & (s55 == `EATING)) |
   ((s55 == `EATING) & (s56 == `EATING)) |
   ((s56 == `EATING) & (s57 == `EATING)) |
   ((s57 == `EATING) & (s58 == `EATING)) |
   ((s58 == `EATING) & (s59 == `EATING)) |
   ((s59 == `EATING) & (s60 == `EATING)) |
   ((s60 == `EATING) & (s61 == `EATING)) |
   ((s61 == `EATING) & (s62 == `EATING)) |
   ((s62 == `EATING) & (s63 == `EATING)) |
   ((s63 == `EATING) & (s0 == `EATING)) ;

assign live = (s0 == `EATING);

philosopher ph0(clk, reset, s0, s1, s63, `READING, r0);
philosopher ph1(clk, reset, s1, s2, s0, `THINKING, r1);
philosopher ph2(clk, reset, s2, s3, s1, `THINKING, r2);
philosopher ph3(clk, reset, s3, s4, s2, `THINKING, r3);
philosopher ph4(clk, reset, s4, s5, s3, `THINKING, r4);
philosopher ph5(clk, reset, s5, s6, s4, `THINKING, r5);
philosopher ph6(clk, reset, s6, s7, s5, `THINKING, r6);
philosopher ph7(clk, reset, s7, s8, s6, `THINKING, r7);
philosopher ph8(clk, reset, s8, s9, s7, `THINKING, r8);
philosopher ph9(clk, reset, s9, s10, s8, `THINKING, r9);
philosopher ph10(clk, reset, s10, s11, s9, `THINKING, r10);
philosopher ph11(clk, reset, s11, s12, s10, `THINKING, r11);
philosopher ph12(clk, reset, s12, s13, s11, `THINKING, r12);
philosopher ph13(clk, reset, s13, s14, s12, `THINKING, r13);
philosopher ph14(clk, reset, s14, s15, s13, `THINKING, r14);
philosopher ph15(clk, reset, s15, s16, s14, `THINKING, r15);
philosopher ph16(clk, reset, s16, s17, s15, `THINKING, r16);
philosopher ph17(clk, reset, s17, s18, s16, `THINKING, r17);
philosopher ph18(clk, reset, s18, s19, s17, `THINKING, r18);
philosopher ph19(clk, reset, s19, s20, s18, `THINKING, r19);
philosopher ph20(clk, reset, s20, s21, s19, `THINKING, r20);
philosopher ph21(clk, reset, s21, s22, s20, `THINKING, r21);
philosopher ph22(clk, reset, s22, s23, s21, `THINKING, r22);
philosopher ph23(clk, reset, s23, s24, s22, `THINKING, r23);
philosopher ph24(clk, reset, s24, s25, s23, `THINKING, r24);
philosopher ph25(clk, reset, s25, s26, s24, `THINKING, r25);
philosopher ph26(clk, reset, s26, s27, s25, `THINKING, r26);
philosopher ph27(clk, reset, s27, s28, s26, `THINKING, r27);
philosopher ph28(clk, reset, s28, s29, s27, `THINKING, r28);
philosopher ph29(clk, reset, s29, s30, s28, `THINKING, r29);
philosopher ph30(clk, reset, s30, s31, s29, `THINKING, r30);
philosopher ph31(clk, reset, s31, s32, s30, `THINKING, r31);
philosopher ph32(clk, reset, s32, s33, s31, `THINKING, r32);
philosopher ph33(clk, reset, s33, s34, s32, `THINKING, r33);
philosopher ph34(clk, reset, s34, s35, s33, `THINKING, r34);
philosopher ph35(clk, reset, s35, s36, s34, `THINKING, r35);
philosopher ph36(clk, reset, s36, s37, s35, `THINKING, r36);
philosopher ph37(clk, reset, s37, s38, s36, `THINKING, r37);
philosopher ph38(clk, reset, s38, s39, s37, `THINKING, r38);
philosopher ph39(clk, reset, s39, s40, s38, `THINKING, r39);
philosopher ph40(clk, reset, s40, s41, s39, `THINKING, r40);
philosopher ph41(clk, reset, s41, s42, s40, `THINKING, r41);
philosopher ph42(clk, reset, s42, s43, s41, `THINKING, r42);
philosopher ph43(clk, reset, s43, s44, s42, `THINKING, r43);
philosopher ph44(clk, reset, s44, s45, s43, `THINKING, r44);
philosopher ph45(clk, reset, s45, s46, s44, `THINKING, r45);
philosopher ph46(clk, reset, s46, s47, s45, `THINKING, r46);
philosopher ph47(clk, reset, s47, s48, s46, `THINKING, r47);
philosopher ph48(clk, reset, s48, s49, s47, `THINKING, r48);
philosopher ph49(clk, reset, s49, s50, s48, `THINKING, r49);
philosopher ph50(clk, reset, s50, s51, s49, `THINKING, r50);
philosopher ph51(clk, reset, s51, s52, s50, `THINKING, r51);
philosopher ph52(clk, reset, s52, s53, s51, `THINKING, r52);
philosopher ph53(clk, reset, s53, s54, s52, `THINKING, r53);
philosopher ph54(clk, reset, s54, s55, s53, `THINKING, r54);
philosopher ph55(clk, reset, s55, s56, s54, `THINKING, r55);
philosopher ph56(clk, reset, s56, s57, s55, `THINKING, r56);
philosopher ph57(clk, reset, s57, s58, s56, `THINKING, r57);
philosopher ph58(clk, reset, s58, s59, s57, `THINKING, r58);
philosopher ph59(clk, reset, s59, s60, s58, `THINKING, r59);
philosopher ph60(clk, reset, s60, s61, s59, `THINKING, r60);
philosopher ph61(clk, reset, s61, s62, s60, `THINKING, r61);
philosopher ph62(clk, reset, s62, s63, s61, `THINKING, r62);
philosopher ph63(clk, reset, s63, s0, s62, `THINKING, r63);

endmodule  // N = 64
