package com.kokkoris.irrigation

import android.content.Context
import android.os.Handler
import android.os.Looper
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import org.eclipse.paho.client.mqttv3.*
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence
import org.json.JSONObject
import java.util.UUID

class MqttRepository(private val context: Context) {
    var state by mutableStateOf(BoardState()); private set
    var message by mutableStateOf("Not connected"); private set
    var config by mutableStateOf(loadConfig()); private set
    private var client: MqttAsyncClient? = null
    private val main = Handler(Looper.getMainLooper())

    fun saveConfig(c: BrokerConfig) {
        config = c
        context.getSharedPreferences("mqtt", Context.MODE_PRIVATE).edit()
            .putString("host", c.host)
            .putInt("port", c.port)
            .putString("user", c.username)
            .putString("pass", c.password)
            .putString("prefix", c.prefix)
            .putBoolean("tls", c.tls)
            .apply()
    }

    private fun loadConfig(): BrokerConfig {
        val p = context.getSharedPreferences("mqtt", Context.MODE_PRIVATE)
        return BrokerConfig(
            p.getString("host", "192.168.1.10")!!,
            p.getInt("port", 1883),
            p.getString("user", "irrigation")!!,
            p.getString("pass", "")!!,
            p.getString("prefix", "garden/irrigation")!!,
            p.getBoolean("tls", false)
        )
    }

    fun connect(c: BrokerConfig = config) {
        saveConfig(c)
        disconnect()
        message = "Connecting…"
        try {
            val uri = (if (c.tls) "ssl" else "tcp") + "://${c.host}:${c.port}"
            val mc = MqttAsyncClient(uri, "android-irrigation-${UUID.randomUUID()}", MemoryPersistence())
            client = mc
            mc.setCallback(object : MqttCallbackExtended {
                override fun connectComplete(reconnect: Boolean, serverURI: String) {
                    mc.subscribe("${c.prefix}/state", 1)
                    main.post {
                        state = state.copy(connected = true)
                        message = "Connected"
                    }
                }
                override fun connectionLost(cause: Throwable?) {
                    main.post {
                        state = state.copy(connected = false)
                        message = "Connection lost"
                    }
                }
                override fun messageArrived(topic: String, m: MqttMessage) {
                    parseState(String(m.payload))
                }
                override fun deliveryComplete(token: IMqttDeliveryToken?) {}
            })
            val o = MqttConnectOptions().apply {
                isAutomaticReconnect = true
                isCleanSession = true
                connectionTimeout = 8
                keepAliveInterval = 30
                if (c.username.isNotBlank()) userName = c.username
                if (c.password.isNotBlank()) password = c.password.toCharArray()
            }
            mc.connect(o, null, object : IMqttActionListener {
                override fun onSuccess(t: IMqttToken?) {}
                override fun onFailure(t: IMqttToken?, e: Throwable?) {
                    main.post { message = e?.message ?: "Connection failed" }
                }
            })
        } catch (e: Exception) {
            message = e.message ?: "Connection failed"
        }
    }

    fun disconnect() {
        try {
            client?.disconnectForcibly()
        } catch (_: Exception) {}
        client = null
        state = state.copy(connected = false)
    }

    private fun publish(suffix: String, payload: String) {
        val c = client
        if (c == null || !c.isConnected) {
            message = "Connect first"
            return
        }
        try {
            c.publish("${config.prefix}/cmd/$suffix", payload.toByteArray(), 1, false)
        } catch (e: Exception) {
            message = e.message ?: "Publish failed"
        }
    }

    fun setZone(i: Int, on: Boolean) = publish("zone${i + 1}", if (on) "ON" else "OFF")
    fun setWeatherBlock(on: Boolean) = publish("weather_block", if (on) "ON" else "OFF")
    fun clearFaults() = publish("clear_faults", "PRESS")

    fun savePlan(index: Int, p: ZonePlan) {
        val j = JSONObject()
            .put("automatic", p.automatic)
            .put("sensor_index", p.sensorIndex)
            .put("start_moisture", p.startMoisture)
            .put("stop_moisture", p.stopMoisture)
            .put("max_runtime_min", p.maxRuntimeMin)
            .put("schedule_enabled", p.scheduleEnabled)
            .put("hour", p.hour)
            .put("minute", p.minute)
            .put("duration_min", p.durationMin)
            .put("days", p.days)
        publish("config/zone${index + 1}", j.toString())
        message = "Plan sent to controller"
    }

    private fun parseState(text: String) {
        try {
            val j = JSONObject(text)
            val plans = if (j.has("plans")) {
                val a = j.getJSONArray("plans")
                List(4) { i ->
                    val p = a.getJSONObject(i)
                    ZonePlan(
                        p.optBoolean("automatic", true),
                        p.optInt("sensor_index", -1),
                        p.optDouble("start_moisture", 30.0).toFloat(),
                        p.optDouble("stop_moisture", 45.0).toFloat(),
                        p.optInt("max_runtime_min", 30),
                        p.optBoolean("schedule_enabled", false),
                        p.optInt("hour", 6),
                        p.optInt("minute", 0),
                        p.optInt("duration_min", 10),
                        p.optInt("days", 127)
                    )
                }
            } else state.plans

            val ns = state.copy(
                connected = true,
                tankState = j.optString("tank_state", "unknown"),
                faults = j.optInt("faults"),
                wellPump = j.optString("well_pump") == "ON",
                weatherBlock = j.optString("weather_block") == "ON",
                irrigationLpm = j.optDouble("irrigation_lpm"),
                wellLpm = j.optDouble("well_lpm"),
                irrigationLiters = j.optDouble("irrigation_liters"),
                wellLiters = j.optDouble("well_liters"),
                zones = List(4) { j.optString("zone${it + 1}") == "ON" },
                soilMoisture = List(2) { if (j.isNull("soil${it + 1}_moisture")) null else j.optDouble("soil${it + 1}_moisture") },
                soilTemperature = List(2) { if (j.isNull("soil${it + 1}_temperature")) null else j.optDouble("soil${it + 1}_temperature") },
                plans = plans
            )
            main.post { state = ns }
        } catch (e: Exception) {
            main.post { message = "Invalid board state: ${e.message}" }
        }
    }
}
