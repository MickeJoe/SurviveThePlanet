(event EventTick (DeltaSeconds)
    (bind t (Utilities|Time|GetGameTimeinSeconds))
    (bind offset (* (Math|Trig|Sin(Radians) (* t 1.1)) 100.0))
    (Transformation|SetRelativeLocation
      :self (Variables|Default|GetMotion)
      :NewLocation (Math|Vector|MakeVector :X (+ 0.000000 offset) :Y 110.000000 :Z 375.000000)
      :bSweep false :bTeleport false))