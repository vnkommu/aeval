(declare-rel inv (Int Int))
(declare-var j Int)
(declare-var j1 Int)
(declare-var d Int)

; for some reason, requires much more time than for nterm_15_cex.smt2

(rule (=> (and (> j d) (> d 1)) (inv j d)))

(rule (=> 
    (and 
        (inv j d)
        (> j d)
        (= j1 (mod j 10))
    )
    (inv j1 d)
  )
)
