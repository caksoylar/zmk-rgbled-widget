# RGB LED を使った LED インジケーター

これは [ZMK module](https://zmk.dev/docs/features/modules) で、3 本の個別 LED で構成された RGB LED を使って状態表示を行うシンプルなウィジェットです。
主にバッテリー残量や BLE 接続状態をミニマルに表示するために使います。

このリポジトリは、オリジナルの [caksoylar/zmk-rgbled-widget](https://github.com/caksoylar/zmk-rgbled-widget) のフォークです。
この環境で使っているのは [yawatajunk/zmk-rgbled-widget](https://github.com/yawatajunk/zmk-rgbled-widget) で、元の機能を維持しつつ、`pwm-leds` を使う基板向けに PWM LED 調光機能を追加しています。

## フォークとライセンス

- 元プロジェクト: [caksoylar/zmk-rgbled-widget](https://github.com/caksoylar/zmk-rgbled-widget)
- この構成で使っているフォーク: [yawatajunk/zmk-rgbled-widget](https://github.com/yawatajunk/zmk-rgbled-widget)
- ライセンス: MIT

このフォークは、元プロジェクトと同じ MIT ライセンスで配布されています。再配布や改変を行う場合は、このリポジトリに含まれる [LICENSE](LICENSE) の著作権表示と MIT ライセンス文を保持してください。

## 機能

- `gpio-leds` を使う従来の GPIO ベース RGB LED 構成に対応
- `pwm-leds` を使う PWM ベース RGB LED 構成に対応
- `CONFIG_RGBLED_WIDGET_BRIGHTNESS` によるウィジェット輝度設定を追加

<details>
  <summary>短いデモ動画</summary>
  起動、プロファイル切替、電源オフまでを含む短いデモ動画です。

  https://github.com/caksoylar/zmk-rgbled-widget/assets/7876996/cfd89dd1-ff24-4a33-8563-2fdad2a828d4
</details>

### バッテリー状態

- 起動時にバッテリー残量に応じて 🟢 / 🟡 / 🔴 を点滅表示します。しきい値は `CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_HIGH` と `CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_LOW` で設定します。
- バッテリー残量が critical 未満になった場合、残量変化のたびに 🔴 を点滅表示します。しきい値は `CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_CRITICAL` です。

### 接続状態

- 起動時のバッテリー表示の後、および BLE プロファイル切替時に、セントラル側では接続状態を点滅表示します。
- 接続済みなら 🔵、アドバタイズ中なら 🟡、未接続なら 🔴 を点滅表示します。
- `CONFIG_RGBLED_WIDGET_CONN_SHOW_USB` を有効にすると、USB が BLE より優先されている場合に上記の代わりにシアンを点滅表示します。
- 分割キーボードのペリフェラル側では、接続中なら 🔵、未接続なら 🔴 を点滅表示します。

### レイヤー状態

有効な最上位レイヤーの表示方法は、次の 2 つから選べます。どちらもデフォルトでは無効です。

- `CONFIG_RGBLED_WIDGET_SHOW_LAYER_CHANGE` を有効にすると、レイヤーが切り替わるたびに、そのレイヤー番号に応じた回数だけシアンを点滅表示します。
  ここで N は 0 始まりのレイヤー番号です。
- `CONFIG_RGBLED_WIDGET_SHOW_LAYER_COLORS` を有効にすると、各レイヤーに固有の色を割り当て、最上位レイヤーの間その色を点灯し続けます。

これらのレイヤー表示は、分割キーボードではセントラル側のみで有効です。ペリフェラル側はレイヤー情報を知らないためです。

> [!TIP]
> バッテリー状態や接続状態をオンデマンドで表示するためのキーマップ behavior については、[下記](#オンデマンド表示)も参照してください。

## インストール

まず、このモジュールを `config/west.yml` に追加します。`projects` に次のようなエントリを足してください。

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: v0.3           # 使用している ZMK のバージョン
      import: app/west.yml
    - name: zmk-rgbled-widget  # <-- 追加するエントリ
      url: https://github.com/yawatajunk/zmk-rgbled-widget
      revision: v0.3-branch    # ZMK のバージョンと合わせること
  self:
    path: config
```

ローカルビルドを含む詳細は、ZMK の [building with modules](https://zmk.dev/docs/features/modules#building-with-modules) を参照してください。

Xiao BLE など、[`rgbled_adapter`](boards/shields/rgbled_adapter) シールドが対応しているボードを使う場合は、`build.yaml` の `shield` に `rgbled_adapter` を追加します。

```yaml
---
include:
  - board: seeeduino_xiao_ble
    shield: hummingbird rgbled_adapter
```

その他のキーボードについては、下の [カスタム board/shield への対応追加](#カスタム-boardshield-への対応追加) を参照してください。

## オンデマンド表示

このモジュールは、バッテリー状態や接続状態を必要なときに表示するための [behavior](https://zmk.dev/docs/keymaps/behaviors) も提供しています。

```dts
#include <behaviors/rgbled_widget.dtsi>  // behavior を使うために必要

/ {
    keymap {
        ...
        some_layer {
            bindings = <
                ...
                &ind_bat  // バッテリー残量を表示
                &ind_con  // 接続状態を表示
                ...
            >;
        };
    };
};
```

対応するキーやコンボを押すと、LED 表示がトリガーされます。
分割キーボードではすべてのパーツで動作するので、有効化したら全パーツへファームを書き込んでください。

> [!NOTE]
> 分割キーボードで、すべてのコントローラが widget に対応していない場合でも behavior 自体は使えます。
> その場合は、`rgbled_adapter` シールド、または `CONFIG_RGBLED_WIDGET` を、有効なパーツだけで使うようにしてください。

## 分割キーボードでのバッテリー表示

デフォルトでは、分割キーボードの各パーツが自分自身のバッテリー残量を 1 回点滅で示します。
ただし、たとえばドングル構成やペリフェラル側に RGB LED widget がない構成では、セントラル側でペリフェラル側のバッテリー残量も表示したい場合があります。
その場合は次の設定が使えます。

- `CONFIG_RGBLED_WIDGET_BATTERY_SHOW_PERIPHERALS`: 自分自身の後に、ペリフェラル側のバッテリー残量も順に表示
- `CONFIG_RGBLED_WIDGET_BATTERY_SHOW_ONLY_PERIPHERALS`: ペリフェラル側のバッテリー残量のみ順に表示

これらは 分割のセントラル側でのみ有効です。
ペリフェラル側の表示順は、分割パーツの初回ペアリング順に従います。
パーツが未接続の場合は、マゼンタ / パープルの点滅で示されます。

## 設定詳細

<details>
<summary>一般設定</summary>

| 名前 | 説明 | デフォルト |
| --- | --- | --- |
| `CONFIG_RGBLED_WIDGET_INTERVAL_MS` | 2 回の点滅間の最小待機時間 (ms) | 500 |
| `CONFIG_RGBLED_WIDGET_BRIGHTNESS` | `pwm-leds` 使用時の輝度パーセント | 100 |

</details>

<details>
<summary>バッテリー関連</summary>

| 名前 | 説明 | デフォルト |
| --- | --- | --- |
| `CONFIG_RGBLED_WIDGET_BATTERY_BLINK_MS` | バッテリー表示の点灯時間 (ms) | 2000 |
| `CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_HIGH` | High とみなすバッテリー残量 (%) | 80 |
| `CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_LOW` | Low とみなすバッテリー残量 (%) | 20 |
| `CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_CRITICAL` | Critical とみなすバッテリー残量 (%) | 5 |
| `CONFIG_RGBLED_WIDGET_BATTERY_COLOR_HIGH` | `LEVEL_HIGH` 以上で使う色 | Green (`2`) |
| `CONFIG_RGBLED_WIDGET_BATTERY_COLOR_MEDIUM` | `LEVEL_LOW` 以上 `LEVEL_HIGH` 未満で使う色 | Yellow (`3`) |
| `CONFIG_RGBLED_WIDGET_BATTERY_COLOR_LOW` | `LEVEL_LOW` 未満で使う色 | Red (`1`) |
| `CONFIG_RGBLED_WIDGET_BATTERY_COLOR_CRITICAL` | `LEVEL_CRITICAL` 未満で使う色 | Red (`1`) |
| `CONFIG_RGBLED_WIDGET_BATTERY_COLOR_MISSING` | バッテリー不明またはペリフェラル未接続時の色 | Magenta (`5`) |

次の 3 つは同時に 1 つだけ有効化できます。後ろ 2 つは 分割のセントラル側でのみ有効です。

| 名前 | 説明 | デフォルト |
| --- | --- | --- |
| `CONFIG_RGBLED_WIDGET_BATTERY_SHOW_SELF` | 自分自身のバッテリー残量のみ表示 | `n` |
| `CONFIG_RGBLED_WIDGET_BATTERY_SHOW_PERIPHERALS` | セントラル側でペリフェラルのバッテリー残量も表示 | `n` |
| `CONFIG_RGBLED_WIDGET_BATTERY_SHOW_ONLY_PERIPHERALS` | セントラル側でペリフェラルのみ表示 | `n` |

</details>

<details>
<summary>接続関連</summary>

| 名前 | 説明 | デフォルト |
| --- | --- | --- |
| `CONFIG_RGBLED_WIDGET_CONN_BLINK_MS` | BLE 接続状態表示の点灯時間 (ms) | 1000 |
| `CONFIG_RGBLED_WIDGET_CONN_SHOW_USB` | USB が優先のとき BLE の代わりに USB 状態を表示 | `n` |
| `CONFIG_RGBLED_WIDGET_CONN_COLOR_CONNECTED` | BLE 接続済み時の色 | Blue (`4`) |
| `CONFIG_RGBLED_WIDGET_CONN_COLOR_ADVERTISING` | BLE アドバタイズ中の色 | Yellow (`3`) |
| `CONFIG_RGBLED_WIDGET_CONN_COLOR_DISCONNECTED` | BLE 未接続時の色 | Red (`1`) |
| `CONFIG_RGBLED_WIDGET_CONN_COLOR_USB` | USB エンドポイント有効時の色 | Cyan (`6`) |

</details>

<details>
<summary>レイヤー関連</summary>

レイヤー表示は非分割キーボード、または 分割キーボードのセントラル側でのみ動作します。

以下は点滅回数ベースのレイヤー表示設定です。

| 名前 | 説明 | デフォルト |
| --- | --- | --- |
| `CONFIG_RGBLED_WIDGET_SHOW_LAYER_CHANGE` | レイヤー変更時に点滅回数で最上位レイヤーを表示 | `n` |
| `CONFIG_RGBLED_WIDGET_LAYER_BLINK_MS` | レイヤー表示の点灯時間 / 待機時間 | 100 |
| `CONFIG_RGBLED_WIDGET_LAYER_COLOR` | レイヤー点滅に使う色 | Cyan (`6`) |
| `CONFIG_RGBLED_WIDGET_LAYER_DEBOUNCE_MS` | レイヤー変更後に表示するまでの待機時間 | 100 |

以下は色ベースのレイヤー表示設定です。

| 名前 | 説明 | デフォルト |
| --- | --- | --- |
| `CONFIG_RGBLED_WIDGET_SHOW_LAYER_COLORS` | 最上位レイヤーを色で常時表示 | `n` |
| `CONFIG_RGBLED_WIDGET_LAYER_0_COLOR` | ベースレイヤーの色 | Black (`0`) |
| `CONFIG_RGBLED_WIDGET_LAYER_1_COLOR` | レイヤー 1 の色 | Red (`1`) |
| `CONFIG_RGBLED_WIDGET_LAYER_2_COLOR` | レイヤー 2 の色 | Green (`2`) |
| `CONFIG_RGBLED_WIDGET_LAYER_3_COLOR` | レイヤー 3 の色 | Yellow (`3`) |
| `CONFIG_RGBLED_WIDGET_LAYER_4_COLOR` | レイヤー 4 の色 | Blue (`4`) |
| `CONFIG_RGBLED_WIDGET_LAYER_5_COLOR` | レイヤー 5 の色 | Magenta (`5`) |
| `CONFIG_RGBLED_WIDGET_LAYER_6_COLOR` | レイヤー 6 の色 | Cyan (`6`) |
| `CONFIG_RGBLED_WIDGET_LAYER_7_COLOR` | レイヤー 7 の色 | White (`7`) |
| `CONFIG_RGBLED_WIDGET_LAYER_xx_COLOR` | レイヤー xx の色 | Black (`0`) |

</details>

<details>
<summary>色番号の対応表</summary>

色設定では次の整数値を使います。

| 色 | 値 |
| --- | --- |
| Black (消灯) | `0` |
| Red | `1` |
| Green | `2` |
| Yellow | `3` |
| Blue | `4` |
| Magenta | `5` |
| Cyan | `6` |
| White | `7` |

</details>

これらの設定は、たとえば `config/hummingbird.conf` のようなキーボード設定ファイルに追加できます。

```ini
CONFIG_RGBLED_WIDGET_INTERVAL_MS=250
CONFIG_RGBLED_WIDGET_BRIGHTNESS=25
CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_HIGH=50
CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_CRITICAL=10
```

PWM 対応フォークを使う場合、`.conf` での主な明るさ制御は `CONFIG_RGBLED_WIDGET_BRIGHTNESS` です。目安は次の通りです。

- `10` から `20`: かなり暗い
- `25` から `40`: 中程度
- `100`: 最大輝度

## カスタム board/shield への対応追加

この widget を使うには、3 つの LED で構成された RGB LED が必要です。一般的には red / green / blue の 3 色です。
まず board または shield 側で LED 定義を行い、その後 `aliases` で RGB LED ノードを参照できるようにします。

以下は、nRF52840 で VCC 側に接続された 3 本の GPIO 制御 LED の例です。

```dts
/ {
    aliases {
        led-red = &led0;
        led-green = &led1;
        led-blue = &led2;
    };

    leds {
        compatible = "gpio-leds";
        status = "okay";
        led0: led_0 {
            gpios = <&gpio0 26 GPIO_ACTIVE_LOW>;  // red LED, connected to P0.26
        };
        led1: led_1 {
            gpios = <&gpio0 30 GPIO_ACTIVE_LOW>;  // green LED, connected to P0.30
        };
        led2: led_2 {
            gpios = <&gpio0 6 GPIO_ACTIVE_LOW>;  // blue LED, connected to P0.06
        };
    };
};
```

LED が GPIO と GND の間に接続されている場合は、`GPIO_ACTIVE_HIGH` を使ってください。

### PWM LED 対応

このフォークは `pwm-leds` を使った PWM 制御 RGB LED にも対応しています。
基板側で RGB LED チャンネルが PWM コントローラに接続されていて、明るさを調整したい場合はこちらを使います。

この構成では、`.conf` 側で PWM ドライバの有効化と widget の明るさを設定し、board または shield の `.overlay` / `.dtsi` 側で PWM ピンや LED ノードを定義します。

`.conf` の例:

```ini
CONFIG_PWM=y
CONFIG_LED_PWM=y
CONFIG_RGBLED_WIDGET=y
CONFIG_RGBLED_WIDGET_BRIGHTNESS=25
```

devicetree の例:

```dts
#include <dt-bindings/pinctrl/nrf-pinctrl.h>
#include <dt-bindings/pwm/pwm.h>

/ {
  aliases {
    led-red = &red_pwm_led;
    led-green = &green_pwm_led;
    led-blue = &blue_pwm_led;
  };
};

&pinctrl {
  pwm0_default: pwm0_default {
    group1 {
      psels = <NRF_PSEL(PWM_OUT0, 0, 26)>, <NRF_PSEL(PWM_OUT1, 0, 30)>,
          <NRF_PSEL(PWM_OUT2, 0, 6)>;
    };
  };

  pwm0_sleep: pwm0_sleep {
    group1 {
      psels = <NRF_PSEL(PWM_OUT0, 0, 26)>, <NRF_PSEL(PWM_OUT1, 0, 30)>,
          <NRF_PSEL(PWM_OUT2, 0, 6)>;
      low-power-enable;
    };
  };
};

&pwm0 {
  status = "okay";
  pinctrl-0 = <&pwm0_default>;
  pinctrl-1 = <&pwm0_sleep>;
  pinctrl-names = "default", "sleep";
};

/ {
  pwm_leds {
    compatible = "pwm-leds";
    status = "okay";

    red_pwm_led: red_pwm_led {
      pwms = <&pwm0 0 PWM_MSEC(10) PWM_POLARITY_INVERTED>;
    };

    green_pwm_led: green_pwm_led {
      pwms = <&pwm0 1 PWM_MSEC(10) PWM_POLARITY_INVERTED>;
    };

    blue_pwm_led: blue_pwm_led {
      pwms = <&pwm0 2 PWM_MSEC(10) PWM_POLARITY_INVERTED>;
    };
  };
};
```

PWM 構成での注意点:

- `CONFIG_RGBLED_WIDGET_BRIGHTNESS` は、widget のすべての表示に対する実行時輝度を制御します。
- `.conf` で `CONFIG_PWM=y` と `CONFIG_LED_PWM=y` を必ず有効にしてください。
- `overlay` または `.dtsi` では、PWM 用 pinctrl の定義、PWM コントローラの有効化、`led-red` / `led-green` / `led-blue` alias の公開が必要です。
- RGB LED が active-low 配線なら `PWM_POLARITY_INVERTED`、そうでなければ `PWM_POLARITY_NORMAL` を使ってください。

最後に、設定ファイルで widget を有効化します。

```ini
CONFIG_RGBLED_WIDGET=y
```
