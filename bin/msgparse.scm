#!/usr/local/bin/ypsilon
(import (pregexp))

(define stdin (current-input-port))
(define *in-proximity* #f)
(define *x* 0)
(define *y* 0)
(define *p* 0)
(define *v* 0)
(define *h* 0)


(define extract bitwise-bit-field)

(define (extend value width)
  (if (bitwise-bit-set? value (- width 1))
      (bitwise-not value)
      value))

(define (extract/extend v start stop)
  (extend (extract v start stop) (- stop start)))

(define (number->binary-string x w)
  (let loop ([bit (- w 1)] [result ""])
    (cond
     [(< bit 0) result]
     [(bitwise-bit-set? x bit)
      (loop (- bit 1) (string-append result "1"))]
     [else 
      (loop (- bit 1) (string-append result "0"))])))

(define (proximity v)
  (let ([tool (extract v 36 48)])
    (display (format "~a in Proximity\n"
                     (cond
                      [(= #x822 tool) "Standard Stylus"]
                      [(= #x812 tool) "Inking Stylus"]
                      [(= #x832 tool) "Stroke Stylus"]
                      [else "other tool"])))))

(define (a-packet v)
  (let ([v (extract v 0 7)]
        [h (extract v 7 14)]
        [p (extract v 14 24)]
        [y (extract v 24 40)]
        [x (extract v 40 56)])
    (display (format "location x = 0x~a y = 0x~a p = 0x~a h = 0x~a v = 0x~a\n"
                         (number->string x 16)
                         (number->string y 16)
                         (number->string p 16)
                         (number->string h 16)
                         (number->string v 16)))
    (if *in-proximity*
        (display (format "error x = 0x~a y = 0x~a p = 0x~a h = 0x~a v = 0x~a\n"
                         (number->string (- x *x*) 16)
                         (number->string (- y *y*) 16)
                         (number->string (- p *p*) 16)
                         (number->string (- h *h*) 16)
                         (number->string (- v *v*) 16))))
    (set! *x* x)
    (set! *y* y)
    (set! *p* p)
    (set! *h* h)
    (set! *v* v)
    (set! *in-proximity* #t)))
      
(define (three-packet v)
  (let ([dv (extract v 0 4)]
        [dh (extract v 4 8)]
        [dp (extract v 8 12)]
        [dy (extract v 12 17)]
        [dx (extract v 17 22)])
    (display (format "~a ~a ~a ~a ~a ~a"
                     (number->binary-string (extract v 22 24) 2)
                     (number->binary-string dx 5)
                     (number->binary-string dy 5)
                     (number->binary-string dp 4)
                     (number->binary-string dh 4)
                     (number->binary-string dv 4)))
    (display (format " | ~a ~a ~a ~a ~a\n"
                     (extend dx 5) (extend dy 5) (extend dp 4) (extend dh 4) (extend dv 4)))
    
    (if *in-proximity*
        (begin
        (set! *v* (+ *v* (extend dv 4)))
        (set! *h* (+ *h* (extend dh 4)))
        (set! *p* (+ *p* (extend dp 4)))
        (set! *y* (+ *y* (extend dy 5)))
        (set! *x* (+ *x* (extend dx 5)))))))

(define (six-packet v)
  (define (modify v1 v2 width)
    (+ (extend (* v1 16) width) (extend v2 width)))
  #;(define (modify v1 v2 width)
    (+ (* (extend v1 width) (bitwise-arithmetic-shift-left 1 width)) (extend v2 width)))
  ;; #;  (define (modify v1 v2 width)
  ;; (+ (* (extend v2 width) (bitwise-arithmetic-shift-left 1 width)) (extend v1 width)))
  ;; #;  (define (modify v1 v2 width)
  ;; (+ (extend v1 width) (extend v2 width)))
  ;; #;  (define (modify v1 v2 width)
  ;; (* (extend v2 width) (abs (extend v1 width))))
  (three-packet (extract v 24 48))
  (three-packet (extract v 0 24))

#;  (let ([dv1 (extract v 0 4)]
        [dh1 (extract v 4 8)]
        [dp1 (extract v 8 12)]
        [dy1 (extract v 12 17)]
        [dx1 (extract v 17 22)]
        [dv2 (extract v 24 28)]
        [dh2 (extract v 28 32)]
        [dp2 (extract v 32 36)]
        [dy2 (extract v 36 41)]
        [dx2 (extract v 41 46)])
    (display (format "~a ~a ~a ~a ~a ~a : "
                     (number->binary-string (extract v 46 48) 2)
                     (number->binary-string dx2 5)
                     (number->binary-string dy2 5)
                     (number->binary-string dp2 4)
                     (number->binary-string dh2 4)
                     (number->binary-string dv2 4)))
    (display (format "~a ~a ~a ~a ~a ~a\n"
                     (number->binary-string (extract v 22 24) 2)
                     (number->binary-string dx1 5)
                     (number->binary-string dy1 5)
                     (number->binary-string dp1 4)
                     (number->binary-string dh1 4)
                     (number->binary-string dv1 4)))
    (if *in-proximity*
        (begin
        (set! *v* (+ *v* (modify dv1 dv2 4)))
        (set! *h* (+ *h* (modify dh1 dh2 4)))
        (set! *p* (+ *p* (modify dp1 dp2 4)))
        (set! *y* (+ *y* (modify dy1 dy2 5)))
        (set! *x* (+ *x* (modify dx1 dx2 5)))))))

(define (prox-out)
  (set! *in-proximity* #f)
  (display "Out of proximity\n"))

(let loop ([packet (get-line stdin)])
  (if (eof-object? packet) #t
      (begin 
        (cond
         [(pregexp-match "[[:xdigit:]]*" packet 5) =>
          (lambda (x)
            (let ([matched-hex (car x)]
                  [matched-val (string->number (car x) 16)])
              (cond
               [(= 14 (string-length matched-hex))
                (proximity matched-val)]
               [(and (= 16 (string-length matched-hex)) (eqv? #\A (string-ref matched-hex 0)))
                (a-packet matched-val)]
               [(and (= 16 (string-length matched-hex))
                     (string=? "FEOO" (substring matched-hex 12 15)))
                (six-packet (extract matched-val 16 64))
                (prox-out)]
               [(= 16 (string-length matched-hex))
                (display (format "other 8 byte packet ~a\n" matched-hex))]
               [(= 12 (string-length matched-hex))
                (six-packet matched-val)]
               [(= 6 (string-length matched-hex))
                (three-packet matched-val)]
               [(= 10 (string-length matched-hex))
                (three-packet (extract matched-val 16 40))
                (prox-out)]
               [(= 4 (string-length matched-hex))
                (prox-out)])))])
        (loop (get-line stdin)))))

                       
