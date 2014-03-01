#!/usr/bin/env ypsilon
(import (pregexp))

(define stdin (current-input-port))
(define *in-proximity* #f)
(define *x* 0)
(define *x-shift* 0)
(define *y* 0)
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
  (let* ([op (if (= 1 (extract delta 3 4)) - +)]
         [delta (extract delta 0 3)]
         [new-value (op value delta)]
         [new-shift
          (case delta
            ((0) (posify (- shift 3)))
            ((1) (posify (- shift 2)))
            ((2 3) (posify (- shift 1)))
            ((4 5) shift)
            ((6) (+ shift 1))
            ((7) (+ shift 2)))])
    (values new-value new-shift)))

(define (tiltify x)
  (- 64 x))

(define (three-packet v)
  (let ([dv (extract v 0 4)]
        [dh (extract v 4 8)]
        [dp (extract v 8 12)]
        [dy (extract v 12 17)]
        [dx (extract v 17 22)])
    (let-values ([(v vs) (decode-tilt dv *v* *v-shift*)])
      (set! *v* v)
      (set! *v-shift* vs))
    (let-values ([(h hs) (decode-tilt dh *h* *h-shift*)])
      (set! *h* h)
      (set! *h-shift* hs))
    (display (format "h ~a : v ~a\n" (tiltify *h*) (tiltify *v*)))
    ))

(define (six-packet v)
  (three-packet (extract v 24 48))
  (three-packet (extract v 0 24)))

(define (proximity v)
  (set! *v-shift* 2)
  (set! *h-shift* 2)
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

                       
