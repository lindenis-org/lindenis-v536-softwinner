
#ifndef _SAMPLE_AI_H_
#define _SAMPLE_AI_H_

#include <pthread.h>
#include <stdint.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <poll.h>

//normal Mode
#define G_SENSOR_SENSI_H_VAL        1.5//600//1.11f   //(1.58f)    for test, normal is 1110
#define G_SENSOR_SENSI_M_VAL        1.8//1.32f   //(1.88f)
#define G_SENSOR_SENSI_L_VAL        2.2//1.61f   //(2.28f)
#define G_SENSOR_SENSI_OFF_VAL      1000//1.61f   //(2.28f)

//Parking Mode
#define G_SENSOR_SENSI_PARKING_H_VAL        1200//150//0.15f
#define G_SENSOR_SENSI_PARKING_M_VAL        1500//200//0.2f
#define G_SENSOR_SENSI_PARKING_L_VAL        1900// MAX
#define G_SENSOR_SENSI_PARKING_OFF_VAL      30000//250//0.25f


#define SUPPORT_SENSORS_NUMBER          (10)
#define ICHAR                           (';')
/*****************************************************************************/

#define ARRAY_SIZE(a)                   (sizeof(a) / sizeof(a[0]))
#define SENSOR_TYPE_ACCELEROMETER                    (1)

#define ID_A                            (0)
#define ID_M                            (1)
#define ID_GY                           (2)
#define ID_L                            (3)
#define ID_PX                           (4)
#define ID_O                            (5)
#define ID_T                            (6)
#define ID_P                            (7)

//#define DEBUG_SENSOR

#define HWROTATION_0                    (0)
#define HWROTATION_90                   (1)
#define HWROTATION_180                  (2)
#define HWROTATION_270                  (3)

#define CONVERT_O                       (1.0f/100.0f)
#define CONVERT_O_Y                     (CONVERT_O)
#define CONVERT_O_P                     (CONVERT_O)
#define CONVERT_O_R                     (CONVERT_O)

/*****************************************************************************/

/*
 * The SENSORS Module
 */

/*****************************************************************************/

#define EVENT_TYPE_ACCEL_X              ABS_X
#define EVENT_TYPE_ACCEL_Y              ABS_Y
#define EVENT_TYPE_ACCEL_Z              ABS_Z

#define EVENT_TYPE_YAW                  ABS_RX
#define EVENT_TYPE_PITCH                ABS_RY
#define EVENT_TYPE_ROLL                 ABS_RZ
#define EVENT_TYPE_ORIENT_STATUS        ABS_WHEEL

#define EVENT_TYPE_MAGV_X               ABS_X
#define EVENT_TYPE_MAGV_Y               ABS_Y
#define EVENT_TYPE_MAGV_Z               ABS_Z

#define EVENT_TYPE_LIGHT                ABS_MISC
#define EVENT_TYPE_TEMPERATURE          ABS_MISC
#define EVENT_TYPE_PROXIMITY            ABS_DISTANCE

#define CONVERT_M                       (1.0f/16.0f)
#define EVENT_TYPE_PRESSURE         ABS_PRESSURE

#define PROXIMITY_THRESHOLD_GP2A        5.0f
/*****************************************************************************/

#define DELAY_OUT_TIME                  0x7FFFFFFF
#define LIGHT_SENSOR_POLLTIME           2000000000
#define SENSOR_STATE_MASK               (0x7FFF)

#define SENSORS_ACCELERATION            (1<<ID_A)
#define SENSORS_MAGNETIC_FIELD          (1<<ID_M)
#define SENSORS_GYROSCOPE               (1<<ID_GY)
#define SENSORS_LIGHT                   (1<<ID_L)
#define SENSORS_PROXIMITY               (1<<ID_PX)
#define SENSORS_ORIENTATION             (1<<ID_O)
#define SENSORS_TEMPERATURE             (1<<ID_T)
#define SENSORS_PRESSURE                (1<<ID_P)


#define SENSORS_ACCELERATION_HANDLE     0
#define SENSORS_MAGNETIC_FIELD_HANDLE   1
#define SENSORS_GYROSCOPE_HANDLE        2
#define SENSORS_LIGHT_HANDLE            3
#define SENSORS_PROXIMITY_HANDLE        4
#define SENSORS_ORIENTATION_HANDLE      5
#define SENSORS_TEMPERATURE_HANDLE      6
#define SENSORS_PRESSURE_HANDLE         7

/*****************************************************************************/

/*****************************************************************************/
#define GRAVITY_EARTH                   (9.80665f)
#define NUMOFACCDATA                    8
#define LSG_LIS35DE                     (15.0f)
#define LSG_MMA7660                     (21.0f)
#define LSG_MXC622X                     (64.0f)
#define LSG_MC32X0                      (64.0f)
#define LSG_LIS3DE_ACC                  (65.0f)
#define LSG_BMA250                      (256.0f)
#define LSG_DAM380                      (256.0f)
#define LSG_STK831X                     (21.0f)
#define LSG_DMARD06                     (256.0f)
#define LSG_GMA301                      (1024.0f)
#define LSG_GMA302                      (1024.0f)
#define LSG_GMA303                      (1024.0f)
#define LSG_DMARD10                     (1024.0f)
#define LSG_MMA8452                     (1024.0f)
#define LSG_KXTIK                       (1024.0f)
#define LSG_MMA865X                     (1024.0f)
#define LSG_LIS3DH_ACC                  (1024.0f)
#define LSG_LSM303D_ACC                 (1024.0f)
#define LSG_AFA750                      (4096.0f)
#define LSG_FXOS8700_ACC                (16384.0f)
#define RANGE_A                         (2*GRAVITY_EARTH)
#define RESOLUTION_A                    (RANGE_A/(256*NUMOFACCDATA))
/*****************************************************************************/

struct sensor_t {

    /* Name of this sensor.
     * All sensors of the same "type" must have a different "name".
     */
    const char*     name;

    /* vendor of the hardware part */
    const char*     vendor;

    /* version of the hardware part + driver. The value of this field
     * must increase when the driver is updated in a way that changes the
     * output of this sensor. This is important for fused sensors when the
     * fusion algorithm is updated.
     */
    int             version;

    /* handle that identifies this sensors. This handle is used to reference
     * this sensor throughout the HAL API.
     */
    int             handle;

    /* this sensor's type. */
    int             type;

    /* maximum range of this sensor's value in SI units */
    float           maxRange;

    /* smallest difference between two values reported by this sensor */
    float           resolution;

    /* rough estimate of this sensor's power consumption in mA */
    float           power;

    /* this value depends on the trigger mode:
     *
     *   continuous: minimum sample period allowed in microseconds
     *   on-change : 0
     *   one-shot  :-1
     *   special   : 0, unless otherwise noted
     */
    int32_t         minDelay;

    /* number of events reserved for this sensor in the batch mode FIFO.
     * If there is a dedicated FIFO for this sensor, then this is the
     * size of this FIFO. If the FIFO is shared with other sensors,
     * this is the size reserved for that sensor and it can be zero.
     */
    uint32_t        fifoReservedEventCount;

    /* maximum number of events of this sensor that could be batched.
     * This is especially relevant when the FIFO is shared between
     * several sensors; this value is then set to the size of that FIFO.
     */
    uint32_t        fifoMaxEventCount;

    /* reserved fields, must be zero */
    void*           reserved[6];
};

/**
 * sensor event data
 */
typedef struct {
    union {
        float v[3];
        struct {
            float x;
            float y;
            float z;
        };
        struct {
            float azimuth;
            float pitch;
            float roll;
        };
    };
    int8_t status;
    uint8_t reserved[3];
} sensors_vec_t;

/**
 * uncalibrated gyroscope and magnetometer event data
 */
typedef struct {
  union {
    float uncalib[3];
    struct {
      float x_uncalib;
      float y_uncalib;
      float z_uncalib;
    };
  };
  union {
    float bias[3];
    struct {
      float x_bias;
      float y_bias;
      float z_bias;
    };
  };
} uncalibrated_event_t;

typedef struct meta_data_event
{
    int32_t what;
    int32_t sensor;
} meta_data_event_t;

/**
 * Union of the various types of sensor data
 * that can be returned.
 */
typedef struct sensors_event_t {
    /* must be sizeof(struct sensors_event_t) */
    int32_t version;

    /* sensor identifier */
    int32_t sensor;

    /* sensor type */
    int32_t type;

    /* reserved */
    int32_t reserved0;

    /* time is in nanosecond */
    int64_t timestamp;

    union {
        union {
            float           data[16];

            /* acceleration values are in meter per second per second (m/s^2) */
            sensors_vec_t   acceleration;

            /* magnetic vector values are in micro-Tesla (uT) */
            sensors_vec_t   magnetic;

            /* orientation values are in degrees */
            sensors_vec_t   orientation;

            /* gyroscope values are in rad/s */
            sensors_vec_t   gyro;

            /* temperature is in degrees centigrade (Celsius) */
            float           temperature;

            /* distance in centimeters */
            float           distance;

            /* light in SI lux units */
            float           light;

            /* pressure in hectopascal (hPa) */
            float           pressure;

            /* relative humidity in percent */
            float           relative_humidity;

            /* uncalibrated gyroscope values are in rad/s */
            uncalibrated_event_t uncalibrated_gyro;

            /* uncalibrated magnetometer values are in micro-Teslas */
            uncalibrated_event_t uncalibrated_magnetic;

            /* this is a special event. see SENSOR_TYPE_META_DATA above.
             * sensors_meta_data_event_t events are all reported with a type of
             * SENSOR_TYPE_META_DATA. The handle is ignored and must be zero.
             */
            meta_data_event_t meta_data;
        };

        union {
            uint64_t        data[8];

            /* step-counter */
            uint64_t        step_counter;
        } u64;
    };
    uint32_t reserved1[4];
} sensors_event_t;


struct sensor_info{
        char sensorName[64];
        char classPath[128];
        char devname[128];
        float priData;
};
struct sensors_data{
        char name[64];
        float lsg;
};
struct sensor_extend_t{
        struct sensors_data sensors;
        struct sensor_t sList;
};

struct g_status{
        bool isUsed;
        bool isFound;
};

struct o_device{
        int isFind;
        char name[32];
};

static struct sensor_extend_t gsensorList[] = {
    {
        {
            "stk831x", LSG_STK831X,
        }, {
            "STK 3-axis Accelerometer",
            "STK",
            1, 0,
            SENSOR_TYPE_ACCELEROMETER,
            RANGE_A,
            GRAVITY_EARTH/21.0f,
            0.30f, 20000,
            0, 0,
            { },
        },
    }, {
        {
            "bma250", LSG_BMA250,
        }, {
            "Bosch 3-axis Accelerometer",
            "Bosch",
            1, 0,
            SENSOR_TYPE_ACCELEROMETER,
            4.0f*9.81f,
            (4.0f*9.81f)/1024.0f,
            0.2f, 0,
            0, 0,
            { },
        },
    },{
        {
            "mma8452", LSG_MMA8452,
        }, {
            "MMA8452 3-axis Accelerometer",
            "Freescale Semiconductor Inc.",
            1, 0,
            SENSOR_TYPE_ACCELEROMETER,
            RANGE_A,
            GRAVITY_EARTH/1024.0f,
            0.30f, 20000,
            0, 0,
            { },
        },
    }, {
        {
            "mma7660", LSG_MMA7660,
        }, {
            "MMA7660 3-axis Accelerometer",
                "Freescale Semiconductor Inc.",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                RANGE_A,
                GRAVITY_EARTH/21.0f,
                0.30f, 20000,
                0, 0,
                { },
        },
    }, {
        {
            "mma865x", LSG_MMA865X,
        }, {
            "MMA865x 3-axis Accelerometer",
                "Freescale Semiconductor Inc.",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                RANGE_A,
                GRAVITY_EARTH/1024.0f,
                0.30f, 20000,
                0, 0,
                { },
        },
    },  {
        {
            "afa750", LSG_AFA750,
        }, {
            "AFA750 3-axis Accelerometer",
                "AFA",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                4.0f*9.81f,
                GRAVITY_EARTH/4096.0f,
                0.8f, 0,
                0, 0,
                { },
        },
    },  {
        {
            "lis3de_acc", LSG_LIS3DE_ACC,
        }, {
            "lis3de detect Accelerometer",
                "ST",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                4.0f*9.81f,
                (4.0f*9.81f)/1024.0f,
                0.2f, 0,
                0, 0,
                { },
        },
    }, {
        {
            "lis3dh_acc", LSG_LIS3DH_ACC,
        }, {
            "lis3dh detect Accelerometer",
                "ST",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                4.0f*9.81f,
                (4.0f*9.81f)/256.0f,
                0.2f, 0,
                0, 0,
                { },
        },
    }, {
        {
            "lsm303d_acc", LSG_LSM303D_ACC,
        }, {
            "lsm303d detect Accelerometer",
                "ST",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                4.0f*9.81f,
                (4.0f*9.81f)/256.0f,
                0.2f, 0,
                0, 0,
                { },
        },
    }, {
        {
            "FreescaleAccelerometer",
                LSG_FXOS8700_ACC,
        }, {
            "Freescale 3-axis Accelerometer",
                "Freescal",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                2.0f*9.81f,
                9.81f/16384.0f,
                0.30f,  20000,
                0, 0,
                { },
        },
    }, {
        {
            "kxtik", LSG_KXTIK,
        }, {
            "Kionix 3-axis Accelerometer",
                "Kionix, Inc.",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                RANGE_A,
                RESOLUTION_A,
                0.23f, 20000,
                0, 0,
                { },
        },
    }, {
        {
            "dmard10", LSG_DMARD10,
        }, {
            "DMARD10 3-axis Accelerometer",
                "DMT",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                (GRAVITY_EARTH * 2.0f),
                GRAVITY_EARTH / 1024,
                0.145f, 0,
                0, 0,
                { },
        },
    }, {
        {
            "dmard06", LSG_DMARD06,
        }, {
            "DMARD06 3-axis Accelerometer",
                "DMT",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                (GRAVITY_EARTH * 3.0f),
                GRAVITY_EARTH/256.0f,
                0.5f, 0,
                0, 0,
                { },
        },
    },{
        {
            "mxc622x", LSG_MXC622X,
        }, {
            "mxc622x 2-axis Accelerometer",
                "MEMSIC",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                2.0,
                GRAVITY_EARTH/64.0f,
                0.005, 0,
                0, 0,
                { },
        },
    },{
        {
            "mc32x0", LSG_MC32X0,
        }, {
            "mc32x0 3-axis Accelerometer",
                "MCXX",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                2.0,
                GRAVITY_EARTH/64.0f,
                0.005, 0,
                0, 0,
               { },
        },
    }, {
        {
            "lis35de", LSG_LIS35DE,
        },  {
                "lis3dh detect Accelerometer",
                "ST",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                4.0f*9.81f,
                (4.0f*9.81f)/15.0f,
                0.2f, 0,
                0, 0,
                { },
            },
    },

        {
        {
            "da380", LSG_DAM380,
        },  {
                "da380 3-axis Accelerometer",
                "Bosch",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                2.0f*9.81f,
                (2.0f*9.81f)/1024.0f,
                0.2f, 0,
                0, 0,
                { },
            },
    },
    {
        {
            "gma30x", LSG_GMA302,
        }, {
            "GMA30x 3-axis Accelerometer",
                "GlobalMems Inc.",
                1, 0,
                SENSOR_TYPE_ACCELEROMETER,
                (GRAVITY_EARTH * 4.0f),
                GRAVITY_EARTH/1024.0f,
                0.145f, 0,
                0,0,
                { },
        },
    },

};

static struct o_device otherDevice[] = {
    {
        0, "sw-",
    }, {
        0, "axp",
    },
};

static struct o_device ctpDevice[] = {
    {
        0, "gt",
    }, {
        0, "gsl",
    }, {
        0, "ft5x"
    },
};

enum Sensibility {
    GSENSOR_CLOSE = 0,
    GSENSOR_LOW,
    GSENSOR_MID,
    GSENSOR_HIGH,
};
#endif  /* _SAMPLE_AI_H_ */

