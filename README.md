# Библиотеки **efektaHappyNode** и **efektaGpiot**

## Для чего?
Ну собственно идея и, частично реализация, скопипазжена у активного пользователя [телеграмм канала @mysensors_rus](https://t.me/mysensors_rus "телеграмм канала @mysensors_rus") Андрея (Andrew, aka Berk, Berkseo, EfektaLab на других популярных DIY-ресурсах).
Идея, по его же словам [в статье на хабре](https://habr.com/ru/post/478960/ "в статье на хабре"), звучит так:
> Многие кто уже использует проект MySensors для построения своего Умного Дома наверное знают о неоптимальной логике работы MySensors на батарейных устройствах. Постоянная отправка презентаций при перезагрузке устройства, неоптимальный режим автоматического восстановления работы устройств в сети, неоптимальное потребление при активированных прерываниях в функции сна, вообще в целом само наличие только двух прерываний во сне. Все обстоятельства как специально намекают на то что основатели проекта Майсенсорс плохо относятся к батарейкам :)

На самом деле, хотелось просто разобраться в коде, для дальнейшего использования в своих проектах. В результате, получилось, чтобы разобраться - пришлось переписать все с нуля. Заодно оформить это в виде библиотечки через подключаемые файлы. Кроме того в библиотеку включены ряд идей, которые обсуждались на канале, но не были реализованы Андреем и добавлено несколько методов, которые по моему скромному усмотрению весьма востребованы.

## Теперь по порядку
- Режим "счастливой ноды" с режимом "бережливой презентации"
- Режим низкопотребляющего сна с любым количеством пинов
- Опциональная закадровая "умная передача состояния батереи" с опциональной передачей вольтажа
- Опциональная закадровая передача уровня сигнала
- Опциональная закадровая передача причины рестарта ноды (естественно при рестарте)
- Редкий умный сон (только через заданное время, независимое, но кратное времени сна) в разработке

Я постарался реализовать не только, чтобы пользоваться было удобно: все обращения в стиле MySensors, минимальный дополнительный код, дублирование обращений ООП простым процедурным стилем, но 
и чтобы реализация была максимально прозрачной, для возможной дальнейшей коллективной доработки. Я конечно Ритчи с Керниганом наизусть не знаю и со Страуструпом в обнимку не сплю, тем не менее С++ люблю
и стараюсь понимать. Кроме того я придерживаюсь некого самопального стандарта, который делает, как мне кажется мой код более легким в понимании и отладке:
- форматирование текста строго табуляциями, выделение логических модулей пустыми строками
- переменный объявлять как можно ближе к месту их использования, глобальные и общеклассовые перемменые должны быть минимизированы. 
- Короткие имена переменных - зло. Переменные именуются по следующему стандарту: `ЭТО_МАКРОСЫ_ПРЕПРОЦЕССОРА` и `КОНСТАНТЫ, ЗНАЧЕНИЕ_ПЕРЕЧИСЛЕНИЙ`, `это_вызовы_из_с_библиотек`, `этоВсёОстальное`.
- Я не смешиваю типы переменных и не стараюсь сам себя запутать, даже в ущерб длинне кода (если тип `bool`, то и присваиваем ему `true`, `false` и применяем логические операторы 'if (bool)', а не 'if (bool == 0)' если тип int, сравниваем его с числом, а не используем неявное преобразование). Да и вообще любое неявное преобразование - зло! 
- По возможности, законченный этап алгоритма должен помещаться на одном экране листинга
- Копипаста - зло. Если некое действие повторяется более одного раза - оно должно быть вынесено в процедуру/функцию/метод

### Режим счастливой ноды
Ну тут лучше, чем Андрей [написал на форуме mysensors.ru](https://mysensors.ru/forum/viewtopic.php?f=5&t=380)
> 1.Новая нода стартует как обычная, задача получить от контролера уникальный ID.
2.После всегда стартует с задержкой доступа к сетапу и лупу в 30 секунд, если за это время не найдена сеть то стартует с псевдо готовым транспортом(это не пассивный режим, тк подтверждения отправки и доставки работают) и с отключенной презентацией(nodePresent()) , тк смысла в ней нет.
3.запускает кастомную реализацию поиска родителя, при подтверждении, производит инициализацию парента, отправляет презентацию, тк ее не было при старте и в тоже время продолжает выполнять какой то заданный функционал, работать с другими нодами.
4.Если гейт становится недоступен уже во время работы, то через несколько неудачных попыток отправки, просто включается кастомный поиск парента, без перевода в режим псевдо готового транспорта, тк во время всех отправок сообщений обнуляется контейнер и библиотека не может запустить самостоятельно поиск парента.
5.Если после поиска парента, парентом становится например не гейт или наоборот парентом был репитер, а стал гейт, то для обновления информации на контролер отправляется кастомное сообщение которое меняет в таблице модуля майсенсорс информацию к какому паренту привязана нода(нет необходимости делать это через презентацию, зачем спамить в радиосеть :))
6.Если после того как гейт стал недоступен и потом была начата инициализация нового парента и она не прошла успешно, тогда уже запускается режим превдо готового транспорта.
7. В пример добавил "как бы" отправку показаний температуры на гейт и отправку просто значения на какую то ноду в сети, номер ноды на которую слать укажите в скетче(это для наглядности примера, можно и не указывать, отправлять будет все равно, даже на несуществующую)
8. При востановлении работы через гейт "счастливая ожидающая нода" сразу старается обновить данные в модуле майсенсорс на контролере.

Мои изменения только в том, что после определенного количества неудачных попыток отправить что-то, режим псевдоготового транспорта запускается одновременно с поиском нового парента. И в случае н-го количества неудач, нода может перегрузиться (защита от "зависания" транспорта) 
Кстати, при передаче нескольких сообщений сеанс считается удачным, если передано (получено потверждение) при передаче хотя бы одного.

__Режим бережливой презентации__ заключается в том, что нода передает гейту свои данные только один раз при старте (или по запросу). Но в ноде алгоритм передачи всех данных *до подтверждения*. 
Если презентацию какого-то сенсора (скетча, парента) не удалось пердать с подтверждением за N попыток, то при каждом следующем сеансе будет повторяться попытка презентации, но только тех данных на которые не получено подтверждение.
Данные о удачных презентациях храняться в памяти пользователя и презентаций до запроса не будет, даже после перезагрузки ноды.

### Использование

В простейшем случае кроме обычных дефайнов среди которых должна обязательно присутствовать настройка `int16_t имяЛюбойПеременной; #define MY_TRANSPORT_WAIT_READY_MS (имяЛюбойПеременной)` необходимо

- подключить библиотеку `#include "efektaHappyNode.h"` ***после*** подключения MySensors, 
- определить переменную типа `CHappyNode` и именем `happyNode(100)`, где в скобках указан адрес пользовательской памяти куда библиотека будет писать служебные данные (5-70 байт)
- в `void before()` вызвать метод `happyNode.init();` или просто `happyInit();`
- в `void setup()` вызвать метод `happyNode.config();` или просто `happyConfig();`
- в `void receive(const MyMessage & message)` включить строчку `if (happyCheckAck(message)) return;` или `if (happyNode.checkAck(message)) return;`
- в `void loop()` первой строчкой вставить `happyProcess();` или `happyNode.run();`
- вместо `void presentation()` использовать `void happyPresentation()` и внутри нее 'happySendSketchInfo' вместо 'sendSketchInfo' и `happyPresent` вместо `present`
- далее везде вместо `send` использовать happySend

Всё! Ваша нода счастлива, а ее презентация бережлива!
    
	#define MY_RADIO_NRF5_ESB

    int16_t myTransportComlpeteMS;
    #define MY_TRANSPORT_WAIT_READY_MS (myTransportComlpeteMS)

    #define CHILD_ID_VIRT 2

    #include <MySensors.h>

    #include "efektaGpiot.h"
    #include "efektaHappyNode.h"

    MyMessage msgVirt(CHILD_ID_VIRT, V_VAR);

    CHappyNode happyNode(100); // Адрес c которого будут храниться пользовательские данные

    void before() {
        happyInit();
    }

    void setup(){
        happyConfig();
    }

    void happyPresentation() {
        happySendSketchInfo("HappyNode test", "V1.0");
        happyPresent(CHILD_ID_VIRT, S_CUSTOM, "STATUS Virtual");
    }

    void loop() {
        happyProcess();
        int8_t wakeupReson = sleep(10000);
        if (wakeupReson == MY_WAKE_UP_BY_TIMER){
            happySend(msgVirt.set((uint16_t)rand()));
        }
    }

    void receive(const MyMessage & message){
        if (happyCheckAck(message)) return;

    }

Для того, чтобы Ваша нода передавала без всяких усилий с вашей стороны уровень сигнала, уровень батареи и причину перезагрузки надо добавить просто такие дефайны:
```cpp
    #define MY_SEND_RSSI 254
    #define MY_SEND_BATTERY 253
    #define MY_SEND_RESET_REASON 252
	
Где числа - это номера сенсоров, по которым будут передаваться данные, естественно они должны отличаться от уже имеющихся сенсоров. Всё остальное (в т.ч. и презентацию сенсоров) библиотека сделает за вас. Если в `MY_SEND_BATTERY` не указан номер сенсора
передается только уровень заряда батареи в %, иначе и уровень в % и вольтаж. 
А при указанном `#MY_SEND_RESET_REASON nnn` можно указать дополнительно `#define MY_RESET_REASON_TEXT` и тогда причина будет передаваться не в числовом виде, а уже ввиде расшифрованого короткого текста.
Кстати, если причину перезагрузки не удалось отправить при запуске, она не стирается и будет отправлена при следующем. Причем, если причины разные - отправятся все.

Второй вариант для самостоятельного управления передачей уровня сигнала и уровня батареи - это не использовать дефайны, а напрямую в нужном месте/времени вызывать методы (на причину перезагрузки - это не распространяется, т.к. управлять ей смысла большого нет - она передается всегда при старте один раз)
К примеру 'loop' может выглядеть так:
```cpp    
    void loop() {
        happyProcess();

        const uint32_t t = millis();
        static uint32_t prev_t = t;

        if (t - prev_t >= 20000) {
            happyNode.sendBattery(253);
            happyNode.sendSignalStrength(254);
            prev_t = t;
        }

        int8_t wakeupReson = sleep(10000);
        if (wakeupReson == MY_WAKE_UP_BY_TIMER){
            happySend(msgVirt.set((uint16_t)rand()));
        }
    }

Для использования низкопотребляющего сна с неограниченным количеством прерываний, необходимо сделать следующее:

- включаем библиотеку `#include "efektaGpiot.h` (естественно `WInterrupts.с` уже должен быть модифицирован) Так же необходимо, чтобы вместе с библиотекой лежали файлы app_util.h, app_gpiote.h, app_gpiote.c
- описываем переменную `CDream interruptedSleep(2);` где число это количество пинов, которое будет будить девайс. Имя переменной менять нельзя.
- в `void setup()`    добавляем описание всех пинов по очереди с помощью `interruptedSleep.addPin` или `addDreamPin(НОМЕР_ПИНА, РЕЖИМ_ВНУТРЕННЕЙ_ПОДТЯЖКИ, СПОСОБ_ПРОБУЖДЕНИЯ);` 
   * РЕЖИМ_ВНУТРЕННЕЙ_ПОДТЯЖКИ может быть `NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_PULLUP` 
   * СПОСОБ_ПРОБУЖДЕНИЯ может быть `CDream::NRF_PIN_HIGH_TO_LOW, CDream::NRF_PIN_LOW_TO_HIGH, CDream::NRF_PIN_GHANGE`
- в конце `void setup()` вызываем `initDream()` или `interruptedSleep.init()`
- далее вместо штатной `sleep` используем `dream(ВРЕМЯ_В_МС, УМНЫЙ_СОН)` или `interruptedSleep.run`

Вот и всё!

### Краткая сводка по классам, методам и функциям
| Метод, функция | Процедурный аналог | Описание |
| -------------- | ------------------ | ----------------------------------------------- |
| `CHappyNode(адрес)` | нет | Конструктор счастливой ноды. Обязательно должен описывать переменную `happyNode`. Адрес = начальному адресу в памяти пользователя (обычно 100)|
| `happyNode.init();` | `happyInit()` | Инициализатор счастливой ноды - должен вызываться из `void before()` |
| `........` | `......` | ............................ |
| `happyNode.sendMsg(сообщение, количество_попыток);` | `happySend(сообщение, количество_попыток)` | Передача сообщения гейту, замена штатному `send`. количество_попыток = максимальному количеству попыток передать и дождаться ответа от гейта или роутера, по умолчанию 1 |
| `happyNode.setMaxTry(что_устанавливаем, количество_попыток);` | нет | Устанавливает максимальное количество попыток, для чего см. ниже |
| CHappyNode::TRY_PRESENT | | для `happyNode.setMaxTry(CHappyNode::TRY_PRESENT, nn)` устанавливает количество попыток передачи презентации каждого элемента (скетча, сенсора, парента), по умолчанию 2 |
| CHappyNode::TRY_SEND_NO_ECHO |  | для `happyNode.setMaxTry(CHappyNode::TRY_SEND_NO_ECHO, nn)` устанавливает сколько сеансов неудачных передач должно быть для перехода в режим поиска нового парента и псевдоготового транспорта, по умолчанию 5 |
| CHappyNode::TRY_NO_PARENT |  | для `happyNode.setMaxTry(CHappyNode::TRY_NO_PARENT, nn)` устанавливает сколько неудачных попыток поиска парента должно быть до ресета ноды, по умолчанию 0 - не перегружать |
