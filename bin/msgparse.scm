#!/usr/bin/env ypsilon
(import (pregexp))

(define stdin (current-input-port))
(define *in-proximity* #f)
(define *x* 0)
(define *x-last* 0)
(define *x-shift* 0)
(define *y* 0)
(define *y-last* 0)
(define *y-shift* 0)
(define *p* 0)
(define *p-shift* 0)
(define *p-flag* 1)
(define *v* 0)
(define *v-shift* 0)
(define *h* 0)
(define *h-shift* 0)

(define extract bitwise-bit-field)

(define (extend value width)
  (if (bitwise-bit-set? value (- width 1))
      (bitwise-not value)
      value))

(define (extract/extend v start stop)
  (extend (extract v start stop) (- stop start)))

(define (<< value shift)
  (bitwise-arithmetic-shift-left value shift))
(define (>> value shift)
  (bitwise-arithmetic-shift-right value shift))
(define & bitwise-and)

(define (bits-fold n-bits width fn acc value)
  (let loop ([lo 0] [hi n-bits] [acc acc])
    (if (>= lo width)
        acc
        (loop hi (+ hi n-bits) (fn (extract value lo hi) acc)))))

(define (number->binary-string x w)
  (bits-fold 1 w
             (lambda (x acc) (string-append (number->string x 2) acc))
             "" x))

(define (number->hex-string x w)
  (bits-fold 4 w
             (lambda (x acc) (string-append (number->string x 16) acc))
             "" x))

(define (posify x)
  (if (> x 0) x 0))

(define (decode-tilt delta value shift)
  (let* ([op (if (equal? 1 (extract delta 3 4)) - +)]
         [delta-magnitude (extract delta 0 3)]
         [new-value (op value (<< delta-magnitude shift))]
         [new-shift
          (case delta-magnitude
            ((0) (posify (- shift 3)))
            ((1) (posify (- shift 2)))
            ((2 3) (posify (- shift 1)))
            ((4 5) shift)
            ((6) (+ shift 1))
            ((7) (+ shift 2)))])
    (values new-value new-shift)))

(define (tiltify x)
  (- 64 x))

(define (decode-loc delta value shift)
  (let* ([dv (extract delta 0 4)]
         [sdv (if (zero? (& delta #x10)) dv (- dv))]
         [new-value (+ value (<< sdv shift))]
         [new-shift
          (case dv
            ((0) (posify (- shift 2)))
            ((1 2 3 4 5 6 7) (posify (- shift 1)))
            ((15) (+ shift 2))
            (else shift))])
    (display (format "delta ~a (~a) shifted by ~a => ~a [shift ~a] "
                     (number->binary-string delta 5) sdv shift (<< sdv shift) new-shift))
    (values new-value new-shift)))

(define (average-loc value last-value)
  (>> (+ value last-value) 1))
              
(define (two-packet v)
  (let ([dp (extract v 0 4)]
        [dy (extract v 4 9)]
        [dx (extract v 9 14)])
    (display "deltas : ")
    (let-values ([(x xs) (decode-loc dx *x-last* *x-shift*)]
                 [(y ys) (decode-loc dy *y-last* *y-shift*)])
      (let ([ax (average-loc x *x-last*)]
            [ay (average-loc y *y-last*)])
        (newline)
        (set! *x-shift* xs)
        (set! *x* ax)
        (set! *x-last* x)
        (set! *y-shift* ys)
        (set! *y* ay)
        (set! *y-last* y)))))
      
(define (three-packet v)
  (two-packet (extract v 8 24))
  (let ([dv (extract v 0 4)]
        [dh (extract v 4 8)])
    #;(display (format "three-packet ~a ~a ~a\n" (number->binary-string v 24) (number->binary-string dh 4) (number->binary-string dv 4)))
    (let-values ([(v vs) (decode-tilt dv *v* *v-shift*)])
      (set! *v* v)
      (set! *v-shift* vs))
    (let-values ([(h hs) (decode-tilt dh *h* *h-shift*)])
      (set! *h* h)
      (set! *h-shift* hs))
    ))

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
        (display (format "error x = ~a y = ~a p = ~a h = ~a v = ~a\n"
                         (number->string (- x *x*))
                         (number->string (- y *y*))
                         (number->string (- p *p*))
                         (number->string (- h *h*))
                         (number->string (- v *v*)))))
    (set! *x* x)
    (set! *y* y)
    ;;(if (not *in-proximity*)
        (begin 
          (set! *x-last* x)
          (set! *y-last* y));;)
    (set! *p* p)
    (set! *h* h)
    (set! *v* v)
      (set! *v-shift* 2)
  (set! *h-shift* 2)
  (set! *x-shift* 4)
  (set! *y-shift* 4)

    (set! *in-proximity* #t)))
      


(define (prox-out)
  (set! *in-proximity* #f)
  (display "Out of proximity\n"))

        
(let loop ([packet (get-line stdin)])
  (if (eof-object? packet) #t
      (begin 
        (cond
         [(pregexp-match "[[:xdigit:]]*" packet 5) =>
          (lambda (x)
            (let* ([matched-hex (car x)]
                   [matched-val (string->number (car x) 16)]
                   [length (string-length matched-hex)])
              (display (format "packet ~a\n" matched-hex))
              (let handle-packet ([index 0])
                (if (< index length)
                    (let ([first-byte (string->number (substring matched-hex index (+ index 2)) 16)])
                      #;(display (format "index ~a\n" index))
                      (cond
                       [(= #x80 (bitwise-and first-byte #xe0))
                        (proximity matched-val)
                        (handle-packet (+ index 14))]
                       [(= #xa0 (bitwise-and first-byte #xe0))
                        (a-packet matched-val)
                        (handle-packet (+ index 16))]
                       [(= #xfe (bitwise-and first-byte #xfe))
                        (prox-out)
                        (handle-packet (+ index 4))]
                       [(= 4 (- length index))
                        (let ([a (* (- length index 4) 4)]
                              [b (* (- length index) 4)])
                          (two-packet (extract matched-val a b))
                          (handle-packet (+ index 4)))]
                       [else
                        (let ([a (* (- length index 6) 4)]
                              [b (* (- length index) 4)])
                          (three-packet (extract matched-val a b))
                          (handle-packet (+ index 6)))]))))))])
        (display (format "~a ~a ~a ~a\n"
                         (number->hex-string *x* 16) *x-shift*
                         (number->hex-string *y* 16) *y-shift*))
        (loop (get-line stdin)))))

                       
