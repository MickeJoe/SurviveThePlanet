(event EventTick (DeltaSeconds)
    (Transformation|AddLocalRotation
      :self (Variables|Default|GetMotion)
      :DeltaRotation (Math|Rotator|MakeRotator :Roll 0.0 :Pitch 0.0 :Yaw (* DeltaSeconds 70.000000))
      :bSweep false :bTeleport false))