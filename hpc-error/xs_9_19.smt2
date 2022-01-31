(set-info :smt-lib-version 2.6)
(set-logic QF_UFLIA)
(set-info :source |
   Wisconsin Safety Analyzer (WiSA) benchmarks translated to SMT format by Hossein Sheini
  |)
(set-info :category "industrial")
(set-info :status unsat)
(declare-fun arg0 () Int)
(declare-fun arg1 () Int)
(declare-fun fmt0 () Int)
(declare-fun fmt1 () Int)
(declare-fun distance () Int)
(declare-fun fmt_length () Int)
(declare-fun adr_lo () Int)
(declare-fun adr_medlo () Int)
(declare-fun adr_medhi () Int)
(declare-fun adr_hi () Int)
(declare-fun format (Int) Int)
(declare-fun percent () Int)
(declare-fun s () Int)
(declare-fun s_count (Int) Int)
(declare-fun x () Int)
(declare-fun x_count (Int) Int)
(assert (let ((?v_1 (+ fmt0 1)) (?v_0 (- (- fmt1 2) fmt0)) (?v_7 (format 0))) (let ((?v_16 (= ?v_7 percent)) (?v_8 (format 1))) (let ((?v_20 (= ?v_8 percent)) (?v_17 (= ?v_8 s)) (?v_51 (= ?v_8 x)) (?v_9 (format 2))) (let ((?v_24 (= ?v_9 percent)) (?v_21 (= ?v_9 s)) (?v_54 (= ?v_9 x)) (?v_10 (format 3))) (let ((?v_28 (= ?v_10 percent)) (?v_25 (= ?v_10 s)) (?v_57 (= ?v_10 x)) (?v_11 (format 4))) (let ((?v_32 (= ?v_11 percent)) (?v_29 (= ?v_11 s)) (?v_60 (= ?v_11 x)) (?v_12 (format 5))) (let ((?v_36 (= ?v_12 percent)) (?v_33 (= ?v_12 s)) (?v_63 (= ?v_12 x)) (?v_13 (format 6))) (let ((?v_40 (= ?v_13 percent)) (?v_37 (= ?v_13 s)) (?v_66 (= ?v_13 x)) (?v_14 (format 7))) (let ((?v_44 (= ?v_14 percent)) (?v_41 (= ?v_14 s)) (?v_69 (= ?v_14 x)) (?v_15 (format 8))) (let ((?v_48 (= ?v_15 percent)) (?v_45 (= ?v_15 s)) (?v_72 (= ?v_15 x)) (?v_18 (and ?v_16 ?v_17)) (?v_19 (s_count 0)) (?v_22 (and ?v_20 ?v_21)) (?v_23 (s_count 1)) (?v_26 (and ?v_24 ?v_25)) (?v_27 (s_count 2)) (?v_30 (and ?v_28 ?v_29)) (?v_31 (s_count 3)) (?v_34 (and ?v_32 ?v_33)) (?v_35 (s_count 4)) (?v_38 (and ?v_36 ?v_37)) (?v_39 (s_count 5)) (?v_42 (and ?v_40 ?v_41)) (?v_43 (s_count 6))) (let ((?v_46 (and ?v_44 ?v_45)) (?v_47 (s_count 7)) (?v_75 (format 9))) (let ((?v_49 (and ?v_48 (= ?v_75 s))) (?v_50 (s_count 8)) (?v_52 (and ?v_16 ?v_51)) (?v_53 (x_count 0)) (?v_55 (and ?v_20 ?v_54)) (?v_56 (x_count 1)) (?v_58 (and ?v_24 ?v_57)) (?v_59 (x_count 2)) (?v_61 (and ?v_28 ?v_60)) (?v_62 (x_count 3)) (?v_64 (and ?v_32 ?v_63)) (?v_65 (x_count 4)) (?v_67 (and ?v_36 ?v_66)) (?v_68 (x_count 5)) (?v_70 (and ?v_40 ?v_69)) (?v_71 (x_count 6)) (?v_73 (and ?v_44 ?v_72)) (?v_74 (x_count 7)) (?v_76 (and ?v_48 (= ?v_75 x))) (?v_77 (x_count 8)) (?v_2 (+ fmt0 0)) (?v_3 (+ fmt0 2)) (?v_4 (+ fmt0 3)) (?v_5 (+ fmt0 4)) (?v_6 (+ fmt0 5))) (not (=> (and (and (and (and (and (and (and (and (and (and (and (= distance 19) (= fmt_length 9)) (= adr_lo 3)) (= adr_medlo 4)) (= adr_medhi 5)) (= adr_hi 6)) (and (and (= percent 37) (= s 115)) (= x 120))) (and (and (and (and (and (and (and (= fmt0 0) (= arg0 (- fmt0 distance))) (>= arg1 fmt0)) (< fmt1 (+ fmt0 (- fmt_length 1)))) (> fmt1 ?v_1)) (>= arg1 (+ arg0 distance))) (< arg1 (+ arg0 (+ distance (- fmt_length 4))))) (= arg1 (+ (+ arg0 (* 4 (s_count ?v_0))) (* 4 (x_count ?v_0)))))) (and (or (or (or (or (or (or (or (or (= fmt1 ?v_2) (= fmt1 ?v_1)) (= fmt1 ?v_3)) (= fmt1 ?v_4)) (= fmt1 ?v_5)) (= fmt1 ?v_6)) (= fmt1 (+ fmt0 6))) (= fmt1 (+ fmt0 7))) (= fmt1 (+ fmt0 8))) (or (or (or (or (or (= arg1 ?v_2) (= arg1 ?v_1)) (= arg1 ?v_3)) (= arg1 ?v_4)) (= arg1 ?v_5)) (= arg1 ?v_6)))) (and (and (and (and (and (and (and (and (or (or (or (or (or (or (or ?v_16 (= ?v_7 s)) (= ?v_7 x)) (= ?v_7 3)) (= ?v_7 4)) (= ?v_7 5)) (= ?v_7 6)) (= ?v_7 255)) (or (or (or (or (or (or (or ?v_20 ?v_17) ?v_51) (= ?v_8 3)) (= ?v_8 4)) (= ?v_8 5)) (= ?v_8 6)) (= ?v_8 255))) (or (or (or (or (or (or (or ?v_24 ?v_21) ?v_54) (= ?v_9 3)) (= ?v_9 4)) (= ?v_9 5)) (= ?v_9 6)) (= ?v_9 255))) (or (or (or (or (or (or (or ?v_28 ?v_25) ?v_57) (= ?v_10 3)) (= ?v_10 4)) (= ?v_10 5)) (= ?v_10 6)) (= ?v_10 255))) (or (or (or (or (or (or (or ?v_32 ?v_29) ?v_60) (= ?v_11 3)) (= ?v_11 4)) (= ?v_11 5)) (= ?v_11 6)) (= ?v_11 255))) (or (or (or (or (or (or (or ?v_36 ?v_33) ?v_63) (= ?v_12 3)) (= ?v_12 4)) (= ?v_12 5)) (= ?v_12 6)) (= ?v_12 255))) (or (or (or (or (or (or (or ?v_40 ?v_37) ?v_66) (= ?v_13 3)) (= ?v_13 4)) (= ?v_13 5)) (= ?v_13 6)) (= ?v_13 255))) (or (or (or (or (or (or (or ?v_44 ?v_41) ?v_69) (= ?v_14 3)) (= ?v_14 4)) (= ?v_14 5)) (= ?v_14 6)) (= ?v_14 255))) (or (or (or (or (or (or (or ?v_48 ?v_45) ?v_72) (= ?v_15 3)) (= ?v_15 4)) (= ?v_15 5)) (= ?v_15 6)) (= ?v_15 255)))) (and (and (and (and (and (and (and (and (and (=> ?v_18 (= ?v_19 1)) (=> (not ?v_18) (= ?v_19 0))) (and (=> ?v_22 (= ?v_23 (+ ?v_19 1))) (=> (not ?v_22) (= ?v_23 ?v_19)))) (and (=> ?v_26 (= ?v_27 (+ ?v_23 1))) (=> (not ?v_26) (= ?v_27 ?v_23)))) (and (=> ?v_30 (= ?v_31 (+ ?v_27 1))) (=> (not ?v_30) (= ?v_31 ?v_27)))) (and (=> ?v_34 (= ?v_35 (+ ?v_31 1))) (=> (not ?v_34) (= ?v_35 ?v_31)))) (and (=> ?v_38 (= ?v_39 (+ ?v_35 1))) (=> (not ?v_38) (= ?v_39 ?v_35)))) (and (=> ?v_42 (= ?v_43 (+ ?v_39 1))) (=> (not ?v_42) (= ?v_43 ?v_39)))) (and (=> ?v_46 (= ?v_47 (+ ?v_43 1))) (=> (not ?v_46) (= ?v_47 ?v_43)))) (and (=> ?v_49 (= ?v_50 (+ ?v_47 1))) (=> (not ?v_49) (= ?v_50 ?v_47))))) (and (and (and (and (and (and (and (and (and (=> ?v_52 (= ?v_53 1)) (=> (not ?v_52) (= ?v_53 0))) (and (=> ?v_55 (= ?v_56 (+ ?v_53 1))) (=> (not ?v_55) (= ?v_56 ?v_53)))) (and (=> ?v_58 (= ?v_59 (+ ?v_56 1))) (=> (not ?v_58) (= ?v_59 ?v_56)))) (and (=> ?v_61 (= ?v_62 (+ ?v_59 1))) (=> (not ?v_61) (= ?v_62 ?v_59)))) (and (=> ?v_64 (= ?v_65 (+ ?v_62 1))) (=> (not ?v_64) (= ?v_65 ?v_62)))) (and (=> ?v_67 (= ?v_68 (+ ?v_65 1))) (=> (not ?v_67) (= ?v_68 ?v_65)))) (and (=> ?v_70 (= ?v_71 (+ ?v_68 1))) (=> (not ?v_70) (= ?v_71 ?v_68)))) (and (=> ?v_73 (= ?v_74 (+ ?v_71 1))) (=> (not ?v_73) (= ?v_74 ?v_71)))) (and (=> ?v_76 (= ?v_77 (+ ?v_74 1))) (=> (not ?v_76) (= ?v_77 ?v_74))))) (and (and (and (and (and (not (= (format fmt1) percent)) (= (format (+ fmt1 1)) s)) (= (format arg1) adr_lo)) (= (format (+ arg1 1)) adr_medlo)) (= (format (+ arg1 2)) adr_medhi)) (= (format (+ arg1 3)) adr_hi)))))))))))))))))
(check-sat)
(exit)
