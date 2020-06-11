(declare-fun x () Int)
(declare-fun y () Int)
(declare-fun z () Int)

; (assert (< (- z) 2)) ; expected solution
(assert (< (- (+ x y) z) 2))
(assert (< (- (+ x y) (* 2 z)) 3))
(assert (< (- (+ x y) (* 3 z)) 4))

(check-sat)
