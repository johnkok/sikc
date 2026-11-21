package com.kokkoris.irrigation

data class BrokerConfig(val host:String="192.168.1.10",val port:Int=1883,val username:String="irrigation",val password:String="",val prefix:String="garden/irrigation",val tls:Boolean=false)
data class ZonePlan(val automatic:Boolean=true,val sensorIndex:Int=-1,val startMoisture:Float=30f,val stopMoisture:Float=45f,val maxRuntimeMin:Int=30,val scheduleEnabled:Boolean=false,val hour:Int=6,val minute:Int=0,val durationMin:Int=10,val days:Int=127)
data class BoardState(
    val connected:Boolean=false,val tankState:String="unknown",val faults:Int=0,val wellPump:Boolean=false,val weatherBlock:Boolean=false,
    val irrigationLpm:Double=0.0,val wellLpm:Double=0.0,val irrigationLiters:Double=0.0,val wellLiters:Double=0.0,
    val displayBrightness:Int=8,
    val zones:List<Boolean> = List(4){false},val soilMoisture:List<Double?> = List(2){null},val soilTemperature:List<Double?> = List(2){null},
    val plans:List<ZonePlan> = List(4){ZonePlan(sensorIndex=if(it<2)it else -1)}
)

