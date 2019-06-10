/*
 *  TOPPERS ECHONET Lite Communication Middleware
 * 
 *  Copyright (C) 2014 Cores Co., Ltd. Japan
 * 
 *  上記著作権者は，以下の(1)～(4)の条件を満たす場合に限り，本ソフトウェ
 *  ア（本ソフトウェアを改変したものを含む．以下同じ）を使用・複製・改
 *  変・再配布（以下，利用と呼ぶ）することを無償で許諾する．
 *  (1) 本ソフトウェアをソースコードの形で利用する場合には，上記の著作
 *      権表示，この利用条件および下記の無保証規定が，そのままの形でソー
 *      スコード中に含まれていること．
 *  (2) 本ソフトウェアを，ライブラリ形式など，他のソフトウェア開発に使
 *      用できる形で再配布する場合には，再配布に伴うドキュメント（利用
 *      者マニュアルなど）に，上記の著作権表示，この利用条件および下記
 *      の無保証規定を掲載すること．
 *  (3) 本ソフトウェアを，機器に組み込むなど，他のソフトウェア開発に使
 *      用できない形で再配布する場合には，次のいずれかの条件を満たすこ
 *      と．
 *    (a) 再配布に伴うドキュメント（利用者マニュアルなど）に，上記の著
 *        作権表示，この利用条件および下記の無保証規定を掲載すること．
 *    (b) 再配布の形態を，別に定める方法によって，TOPPERSプロジェクトに
 *        報告すること．
 *  (4) 本ソフトウェアの利用により直接的または間接的に生じるいかなる損
 *      害からも，上記著作権者およびTOPPERSプロジェクトを免責すること．
 *      また，本ソフトウェアのユーザまたはエンドユーザからのいかなる理
 *      由に基づく請求からも，上記著作権者およびTOPPERSプロジェクトを
 *      免責すること．
 * 
 *  本ソフトウェアは，無保証で提供されているものである．上記著作権者お
 *  よびTOPPERSプロジェクトは，本ソフトウェアに関して，特定の使用目的
 *  に対する適合性も含めて，いかなる保証も行わない．また，本ソフトウェ
 *  アの利用により直接的または間接的に生じたいかなる損害に関しても，そ
 *  の責任を負わない．
 * 
 *  @(#) $Id$
 */

#ifndef ECHONET_CLASS_H
#define ECHONET_CLASS_H

/* センサー関連機器 */
#define EOJ_X1_SENSOR 0x00

/* 空調関連機器 */
#define EOJ_X1_AIR_CONDITIONER 0x01

/* 住宅・設備関連機器 */
#define EOJ_X1_AMENITY 0x02

/* 調理・家事関連機器 */
#define EOJ_X1_HOUSEWORK 0x03

/* 健康関連機器 */
#define EOJ_X1_WELLNESS 0x04

/* 管理・操作関連機器 */
#define EOJ_X1_CONTROLLER 0x05

/* AV関連機器 */
#define EOJ_X1_AUDIO_VISUAL 0x06

/* プロファイル */
#define EOJ_X1_PROFILE 0x0E

/* ユーザー定義 */
#define EOJ_X1_USER_DEFINED 0x0F

/*
 * センサ関連機器クラスグループ *
 */
/* ガス漏れセンサ */
#define EOJ_X2_GAS_LEAK_SENSOR 0x01

/* 防犯センサ */
#define EOJ_X2_CRIME_PREVENTION_SENSOR 0x02

/* 非常ボタン */
#define EOJ_X2_EMERGENCY_BUTTON 0x03

/* 救急用センサ */
#define EOJ_X2_FIRST_AID_SENSOR 0x04

/* 地震センサ */
#define EOJ_X2_EARTHQUAKE_SENSOR 0x05

/* 漏電センサ */
#define EOJ_X2_ELECTRIC_LEAK_SENSOR 0x06

/* 人体検知センサ */
#define EOJ_X2_HUMAN_DETECTION_SENSOR 0x07

/* 来客センサ */
#define EOJ_X2_VISITOR_SENSOR 0x08

/* 呼び出しセンサ */
#define EOJ_X2_CALL_SENSOR 0x09

/* 結露センサ */
#define EOJ_X2_CONDENSATION_SENSOR 0x0A

/* 空気汚染センサ */
#define EOJ_X2_AIR_POLLUTION_SENSOR 0x0B

/* 酸素センサ */
#define EOJ_X2_OXYGEN_SENSOR 0x0C

/* 照度センサ */
#define EOJ_X2_ILLUMINANCE_SENSOR 0x0D

/* 音センサ */
#define EOJ_X2_SOUND_SENSOR 0x0E

/* 投函センサ */
#define EOJ_X2_MAILING_SENSOR 0x0F

/* 重荷センサ */
#define EOJ_X2_WEIGHT_SENSOR 0x10

/* 温度センサ */
#define EOJ_X2_TEMPERATURE_SENSOR 0x11

/* 湿度センサ */
#define EOJ_X2_HUMIDITY_SENSOR 0x12

/* 雨センサ */
#define EOJ_X2_RAIN_SENSOR 0x13

/* 水位センサ */
#define EOJ_X2_WATER_LEVEL_SENSOR 0x14

/* 風呂水位センサ */
#define EOJ_X2_BATH_WATER_LEVEL_SENSOR 0x15

/* 風呂沸き上がりセンサ */
#define EOJ_X2_BATH_HEATING_STATUS_SENSOR 0x16

/* 水漏れセンサ */
#define EOJ_X2_WATER_LEAK_SENSOR 0x17

/* 水あふれセンサ */
#define EOJ_X2_WATER_OVERFLOW_SENSOR 0x18

/* 火災センサ */
#define EOJ_X2_FIRE_SENSOR 0x19

/* タバコ煙センサ */
#define EOJ_X2_CIGARETTE_SMOKE_SENSOR 0x1A

/* ＣＯ２センサ */
#define EOJ_X2_CO2_SENSOR 0x1B

/* ガスセンサ */
#define EOJ_X2_GAS_SENSOR 0x1C

/* ＶＯＣセンサ */
#define EOJ_X2_VOC_SENSOR 0x1D

/* 差圧センサ */
#define EOJ_X2_DIFFERENTIAL_PRESSURE_SENSOR 0x1E

/* 風速センサ */
#define EOJ_X2_AIR_SPEED_SENSOR 0x1F

/* 臭いセンサ */
#define EOJ_X2_ODOR_SENSOR 0x20

/* 炎センサ */
#define EOJ_X2_FLAME_SENSOR 0x21

/* 電力量センサ */
#define EOJ_X2_ELECTRIC_ENERGY_SENSOR 0x22

/* 電流量センサ */
#define EOJ_X2_CURRENT_VALUE_SENSOR 0x23

/* 昼光センサ */
#define EOJ_X2_DAYLIGHT_SENSOR 0x24

/* 水流量センサ */
#define EOJ_X2_WATER_FLOW_RATE_SENSOR 0x25

/* 微動センサ */
#define EOJ_X2_MICROMOTION_SENSOR 0x26

/* 通過センサ */
#define EOJ_X2_PASSAGE_SENSOR 0x27

/* 在床センサ */
#define EOJ_X2_BED_PRESENCE_SENSOR 0x28

/* 開閉センサ */
#define EOJ_X2_OPEN_CLOSE_SENSOR 0x29

/* 活動量センサ */
#define EOJ_X2_ACTIVITY_AMOUNT_SENSOR 0x2A

/* 人体位置センサ */
#define EOJ_X2_HUMAN_BODY_LOCATION_SENSOR 0x2B

/* 雪センサ */
#define EOJ_X2_SNOW_SENSOR 0x2C

/*
 * 空調関連機器クラスグループ *
 */
/* 家庭用エアコン */
#define EOJ_X2_HOME_AIR_CONDITIONER 0x30

/* 冷風機 */
#define EOJ_X2_COLD_BLASTER 0x31

/* 扇風機 */
#define EOJ_X2_ELECTRIC_FAN 0x32

/* 換気扇 */
#define EOJ_X2_VENTILATION_FAN 0x33

/* 空調換気扇 */
#define EOJ_X2_AIR_CONDITIONING_VENTILATION_FAN 0x34

/* 空気清浄器 */
#define EOJ_X2_AIR_CLEANER 0x35

/* 冷風扇 */
#define EOJ_X2_COLD_BLAST_FAN 0x36

/* サーキュレータ */
#define EOJ_X2_CIRCULATOR 0x37

/* 除湿機 */
#define EOJ_X2_DEHUMIDIFIER 0x38

/* 加湿器 */
#define EOJ_X2_HUMIDIFIER 0x39

/* 天井扇 */
#define EOJ_X2_CEILING_FAN 0x3A

/* 電気こたつ */
#define EOJ_X2_ELECTRIC_KOTATSU 0x3B

/* 電気あんか */
#define EOJ_X2_ELECTRIC_HEATING_PAD 0x3C

/* 電気毛布 */
#define EOJ_X2_ELECTRIC_BLANKET 0x3D

/* ストーブ */
#define EOJ_X2_SPACE_HEATER 0x3E

/* パネルヒータ */
#define EOJ_X2_PANEL_HEATER 0x3F

/* 電気カーペット */
#define EOJ_X2_ELECTRIC_CARPET 0x40

/* フロアヒータ */
#define EOJ_X2_FLOOR_HEATER 0x41

/* 電気暖房器 */
#define EOJ_X2_ELECTRIC_HEATER 0x42

/* ファンヒータ */
#define EOJ_X2_FAN_HEATER 0x43

/* 充電器 */
#define EOJ_X2_BATTERY_CHARGER 0x44

/* 業務用パッケージエアコン室内機 */
#define EOJ_X2_PACKAGE_TYPE_COMMERCIAL_AIR_CONDITIONER_INDOOR_UNIT 0x45

/* 業務用パッケージエアコン室外機 */
#define EOJ_X2_PACKAGE_TYPE_COMMERCIAL_AIR_CONDITIONER_OUTDOOR_UNIT 0x46

/* 業務用パッケージエアコン蓄熱ユニット */
#define EOJ_X2_PACKAGE_TYPE_COMMERCIAL_AIR_CONDITIONER_THERMAL_STORAGE_UNIT 0x47

/* 業務用ファンコイルユニット */
#define EOJ_X2_COMMERCIAL_FAN_COIL_UNIT 0x48

/* 業務用空調冷熱源(チラー) */
#define EOJ_X2_COMMERCIAL_AIR_CONDITIONING_COLD_SOURCE_CHILLER 0x49

/* 業務用空調温熱源(ボイラー) */
#define EOJ_X2_COMMERCIAL_AIR_CONDITIONING_HEAT_SOURCE_BOILER 0x50

/* 業務用空調VAV */
#define EOJ_X2_AIR_CONDITIONING_VAV_FOR_COMMERCIAL_APPLICATION 0x51

/* 業務用空調エアハンドリングユニット */
#define EOJ_X2_AIR_HANDLING_UNIT_AIR_CONDITIONING_FOR_COMMERCIAL_APPLICATION 0x52

/* ユニットクーラー */
#define EOJ_X2_UNIT_COOLER 0x53

/* 業務用コンデンシングユニット */
#define EOJ_X2_CONDENSING_UNIT_FOR_COMMERCIAL_APPLICATION 0x54

/*
 * 住宅・設備関連機器クラスグループ *
 */
/* 電動ブラインド */
#define EOJ_X2_ELECTRICALLY_OPERATED_SHADE 0x60

/* 電動シャッター */
#define EOJ_X2_ELECTRICALLY_OPERATED_SHUTTER 0x61

/* 電動カーテン */
#define EOJ_X2_ELECTRICALLY_OPERATED_CURTAIN 0x62

/* 電動雨戸 */
#define EOJ_X2_ELECTRICALLY_OPERATED_STORM_WINDOW 0x63

/* 電動ガレージ */
#define EOJ_X2_ELECTRICALLY_OPERATED_DOOR 0x64

/* 電動天窓 */
#define EOJ_X2_ELECTRICALLY_OPERATED_SKYLIGHT 0x65

/* オーニング（日よけ） */
#define EOJ_X2_AWNING 0x66

/* 散水器（庭用） */
#define EOJ_X2_GARDEN_SPRINKLER 0x67

/* 散水器（火災用） */
#define EOJ_X2_FIRE_SPRINKLER 0x68

/* 噴水 */
#define EOJ_X2_FOUNTAIN 0x69

/* 瞬間湯沸器 */
#define EOJ_X2_INSTANTANEOUS_WATER_HEATERS 0x6A

/* 電気温水器 */
#define EOJ_X2_ELECTRIC_WATER_HEATER 0x6B

/* 太陽熱温水器 */
#define EOJ_X2_SOLAR_WATER_HEATER 0x6C

/* 循環ポンプ */
#define EOJ_X2_CIRCULATION_PUMP 0x6D

/* 電気便座（温水洗浄便座、暖房便座など） */
#define EOJ_X2_BIDET_EQUIPPED_TOILET 0x6E

/* 電気錠 */
#define EOJ_X2_ELECTRIC_KEY 0x6F

/* ガス元弁 */
#define EOJ_X2_GAS_LINE_VALVE 0x70

/* ホームサウナ */
#define EOJ_X2_HOME_SAUNA 0x71

/* 瞬間式給湯器 */
#define EOJ_X2_HOT_WATER_GENERATOR 0x72

/* 浴室暖房乾燥機 */
#define EOJ_X2_BATHROOM_DRYER 0x73

/* ホームエレベータ */
#define EOJ_X2_HOME_ELEVATOR 0x74

/* 電動間仕切り */
#define EOJ_X2_ELECTRICALLY_OPERATED_ROOM_DIVIDER 0x75

/* 水平トランスファ */
#define EOJ_X2_HORIZONTAL_TRANSFER 0x76

/* 電動物干し */
#define EOJ_X2_ELECTRICALLY_OPERATED_CLOTHES_DRYING_POLE 0x77

/* 浄化槽 */
#define EOJ_X2_SEPTIC_TANK 0x78

/* 住宅用太陽光発電 */
#define EOJ_X2_HOME_SOLAR_POWER_GENERATION 0x79

/* 冷温水熱源機 */
#define EOJ_X2_COLD_AND_HOT_WATER_HEAT_SOURCE_EQUIPMENT 0x7A

/* 床暖房 */
#define EOJ_X2_FLOOR_HEATING 0x7B

/* 燃料電池 */
#define EOJ_X2_FUEL_CELL 0x7C

/* 蓄電池 */
#define EOJ_X2_STORAGE_BATTERY 0x7D

/* 電気自動車充放電システム */
#define EOJ_X2_ELECTRIC_VEHICLE 0x7E

/* 電力量メータ */
#define EOJ_X2_ELECTRIC_ENERGY_METER 0x80

/* 水流量メータ */
#define EOJ_X2_WATER_FLOW_METER 0x81

/* ガスメータ */
#define EOJ_X2_GAS_METER 0x82

/* LPガスメータ */
#define EOJ_X2_LP_GAS_METERS 0x83

/* 時計 */
#define EOJ_X2_CLOCK 0x84

/* 自動ドア */
#define EOJ_X2_AUTOMATIC_DOOR 0x85

/* 業務用エレベータ */
#define EOJ_X2_COMMERCIAL_ELEVATOR 0x86

/* 分電盤メータリング */
#define EOJ_X2_DISTRIBUTION_PANEL_METERING 0x87

/* スマート電力量メータ */
#define EOJ_X2_SMART_ELECTRIC_ENERGY_METER 0x88

/* スマートガスメータ */
#define EOJ_X2_SMART_GAS_METER 0x89

/* 一般照明 */
#define EOJ_X2_GENERAL_LIGHTING_CLASS 0x90

/* 非常照明 */
#define EOJ_X2_EMERGENCY_LIGHTING 0x99

/* 設備照明 */
#define EOJ_X2_EQUIPMENT_LIGHT 0x9D

/* ブザー */
#define EOJ_X2_BUZZER 0xA0

/*
 * 調理・家事関連機器クラスグループ *
 */
/* コーヒーメーカ */
#define EOJ_X2_COFFEE_MACHINE 0xB0

/* コーヒーミル */
#define EOJ_X2_COFFEE_MILL 0xB1

/* 電気ポット */
#define EOJ_X2_ELECTRIC_HOT_WATER_POT 0xB2

/* 電気こんろ */
#define EOJ_X2_ELECTRIC_STOVE 0xB3

/* トースタ */
#define EOJ_X2_TOASTER 0xB4

/* ジューサ・ミキサ */
#define EOJ_X2_JUICER_FOOD_MIXER 0xB5

/* フードプロセッサ */
#define EOJ_X2_FOOD_PROCESSOR 0xB6

/* 冷凍冷蔵庫 */
#define EOJ_X2_REFRIGERATOR_FREEZER 0xB7

/* オーブンレンジ */
#define EOJ_X2_COMBINATION_MICROWAVE_OVEN 0xB8

/* クッキングヒータ */
#define EOJ_X2_COOKING_HEATER 0xB9

/* オーブン */
#define EOJ_X2_OVEN 0xBA

/* 炊飯器 */
#define EOJ_X2_RICE_COOKER 0xBB

/* 電子ジャー */
#define EOJ_X2_ELECTRONIC_JAR 0xBC

/* 食器洗い機 */
#define EOJ_X2_DISH_WASHER 0xBD

/* 食器乾燥機 */
#define EOJ_X2_DISH_DRYER 0xBE

/* 電気もちつき機 */
#define EOJ_X2_ELECTRIC_RICE_CAKE_COOKER 0xBF

/* 保温機 */
#define EOJ_X2_KEEP_WARM_MACHINE 0xC0

/* 精米機 */
#define EOJ_X2_RICE_MILL 0xC1

/* 自動製パン機 */
#define EOJ_X2_AUTOMATIC_BREAD_COOKER 0xC2

/* スロークッカー */
#define EOJ_X2_SLOW_COOKER 0xC3

/* 電気漬物機 */
#define EOJ_X2_ELECTRIC_PICKLES_COOKER 0xC4

/* 洗濯機 */
#define EOJ_X2_WASHING_MACHINE 0xC5

/* 衣類乾燥機 */
#define EOJ_X2_CLOTH_DRYER 0xC6

/* 電気アイロン */
#define EOJ_X2_ELECTRIC_IRON 0xC7

/* ズボンプレッサ */
#define EOJ_X2_TROUSER_PRESS 0xC8

/* ふとん乾燥機 */
#define EOJ_X2_FUTON_DRYER 0xC9

/* 小物・くつ乾燥機 */
#define EOJ_X2_SMALL_ARTICLE_SHOES_DRYER 0xCA

/* 電気掃除機（セントラルクリーナ含む） */
#define EOJ_X2_ELECTRIC_VACUUM_CLEANER 0xCB

/* ディスポーザ */
#define EOJ_X2_DISPOSER 0xCC

/* 電気蚊取り機 */
#define EOJ_X2_ELECTRIC_MOSQUITO_CATCHER 0xCD

/* 業務用ショーケース */
#define EOJ_X2_COMMERCIAL_SHOW_CASE 0xCE

/* 業務用冷蔵庫 */
#define EOJ_X2_COMMERCIAL_REFRIGERATOR 0xCF

/* 業務用ホットケース */
#define EOJ_X2_COMMERCIAL_HOT_CASE 0xD0

/* 業務用フライヤー */
#define EOJ_X2_COMMERCIAL_FRYER 0xD1

/* 業務用電子レンジ */
#define EOJ_X2_COMMERCIAL_MICROWAVE_OVEN 0xD2

/* 洗濯乾燥機 */
#define EOJ_X2_WASHER_AND_DRYER 0xD3

/*
 * 健康関連機器クラスグループ *
 */
/* 体重計 */
#define EOJ_X2_WEIGHING_MACHINE 0x01

/* 体温計 */
#define EOJ_X2_CLINICAL_THERMOMETER 0x02

/* 血圧計 */
#define EOJ_X2_BLOOD_PRESSURE_METER 0x03

/* 血糖値計 */
#define EOJ_X2_BLOOD_SUGAR_METER 0x04

/* 体脂肪計 */
#define EOJ_X2_BODY_FAT_METER 0x05

/*
 * 管理・操作関連機器クラスグループ *
 */
/* セキュア通信用共有鍵設定ノード */
#define EOJ_X2_SECURE_COMMUNICATION_SHARED_KEY_SETUP_NODE 0xFC

/* スイッチ（JEMA/HA端子対応） */
#define EOJ_X2_SWITCH 0xFD

/* 携帯端末 */
#define EOJ_X2_PORTABLE_TERMINAL 0xFE

/* コントローラ */
#define EOJ_X2_CONTROLLER 0xFF

/*
 * ＡＶ関連機器クラスグループ *
 */
/* ディスプレー */
#define EOJ_X2_DISPLAY 0x01

/* テレビ */
#define EOJ_X2_TELEVISION 0x02

/*
 * プロファイルクラスグループ *
 */
/* ノードプロファイル */
#define EOJ_X2_NODE_PROFILE 0xF0

#endif /* ECHONET_CLASS_H */
