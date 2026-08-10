;; VESC LispBM — v2 (direct current control + speed governor)
;; M5Stack sends scaled current (int16 amps) over CAN EID 0x101.
;; M5Stack sends winch state (0-6) over CAN SID 10.
;; Features: current ramp (0.4s, 25A/sec), 3s watchdog, ERPM limit for state 0-1.
;; Paste this into VESC Tool > LispBM Editor > Upload.

(define rate 100)      ; Update rate in Hz
(def ramp-time 0.4)    ; Faster ramp time in seconds
(def ramp-rate 100)    ; Increase update rate in Hz
(def ramp-step (/ 10.0 (* ramp-rate ramp-time))) ; Calculate step size for ramping current

; Initialize ramping state variables
(def current (get-current 1))
(def target-current current)
(def winch-state 0)           ; Current winch state (0-6), received via CAN SID 10
(def last-winch-state -1)     ; Track state changes for ERPM limit updates
(def last-can-time (systime)) ; Watchdog: timestamp of last received CAN command

; Handler for EID CAN-frames
(defun proc-eid (id data)
    (if (= id 0x00000101u32) ; Check if the incoming CAN ID is 0x00000101
        (progn
            (setq target-current (bufget-i16 data 0 t)) ; Read int16 big-endian: target amps from M5Stack
            (setq last-can-time (systime)) ; Reset watchdog
            (free data) ; Free the data to prevent memory leaks
        )
    )
)

; Handler for SID CAN-frames (state from M5Stack)
(defun proc-sid (id data)
    (if (= id 10)
        (progn
            (setq winch-state (bufget-i32 data 0))
            (free data)
        )
    )
)

; Event handler to continuously process incoming CAN messages
(defun event-handler ()
    (loopwhile t
        (recv
            ((event-can-eid (? id) . (? data)) (proc-eid id data))
            ((event-can-sid (? id) . (? data)) (proc-sid id data))
            (_ nil)
        )
    )
)

; Register and enable the event handler
(event-register-handler (spawn 150 event-handler))
(event-enable 'event-can-eid)
(event-enable 'event-can-sid)

; Main loop
(loopwhile t
  (progn
    ; Watchdog: if no CAN command received for >1 second, zero the current
    (if (> (secs-since last-can-time) 3.0)
        (setq target-current 0))

    ; Update current towards target-current
    (if (< current target-current)
        (setq current (+ current ramp-step))
        (if (> current target-current)
            (setq current target-current)))
    ; Ensure current does not overshoot the target
    (if (< current target-current)
        (if (> (+ current ramp-step) target-current)
            (setq current target-current)))

    ; Speed limit: slow tensioning for state 0-1, full speed for tow states
    ; Only update when state changes to avoid unnecessary conf-set calls
    (if (not (= winch-state last-winch-state))
        (progn
            (if (<= winch-state 1)
                (conf-set 'l-max-erpm 5000)
                (conf-set 'l-max-erpm 100000))
            (setq last-winch-state winch-state)))

    (set-current current)

    ; Get data
    (define getdist (get-dist 1))
    (define getbat (get-batt))
    (define gettemp (get-temp-mot))
    (define getcurrant (get-current 2))

    ; Format the values with 3 decimal places
    (define formatted-dist (str-from-n getdist "%.3f"))
    (define formatted-bat (str-from-n getbat "%.3f"))
    (define formatted-temp (str-from-n gettemp "%.3f"))
    (define formatted-currant (str-from-n getcurrant "%.3f"))

    ; Create buffers to store the integer values
    (define distance-buffer (bufcreate 4)) ; Buffer for distance
    (define current-buffer (bufcreate 4))  ; Buffer for current
    (define temp-buffer (bufcreate 4))     ; Buffer for temp

    ; Store the integer values in their respective buffers
    (bufset-i32 distance-buffer 0 (to-i getdist))
    (bufset-i32 current-buffer 0 (to-i getcurrant))
    (bufset-i32 temp-buffer 0 (to-i gettemp))

    ; Send separate CAN messages with different IDs
    (can-send-sid 7 distance-buffer) ; Send distance with ID 7
    (can-send-sid 8 current-buffer)  ; Send current with ID 8
    (can-send-sid 9 temp-buffer)     ; Send temp with ID 9

    (sleep 0.1)
  )
)
