(declare-fun x () Int)
(declare-fun y () Int)

; (assert (>= x 0)) ; expected solution
(assert (>= (+ y x) 0))
(assert (>= (+ y (* 2 x)) 0))
(assert (>= (+ y (* 3 x)) 0))

(check-sat)
