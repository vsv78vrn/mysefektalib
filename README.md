### Для чего?
Ну собственно идея и, частично реализация, скопипазжена у активного пользователя [телеграмм канала @mysensors_rus](https://t.me/mysensors_rus "телеграмм канала @mysensors_rus") Андрея (Andrew, aka Berk, Berkseo, EfektaLab на других популярных DIY-ресурсах).
Идея, по его же словам [в статье на хабре](https://habr.com/ru/post/478960/ "в статье на хабре"), звучит так:
> Многие кто уже использует проект MySensors для построения своего Умного Дома наверное знают о неоптимальной логике работы MySensors на батарейных устройствах. Постоянная отправка презентаций при перезагрузке устройства, неоптимальный режим автоматического восстановления работы устройств в сети, неоптимальное потребление при активированных прерываниях в функции сна, вообще в целом само наличие только двух прерываний во сне. Все обстоятельства как специально намекают на то что основатели проекта Майсенсорс плохо относятся к батарейкам :)

Собственно, хотелось просто разобраться в коде, для дальнейшего использования в своих проектах. В результате, получилось, чтобы разобраться - пришлось переписать все с нуля. Заодно оформить это в виде библиотечки через подключаемые файлы. Кроме того в библиотеку включены ряд идей, которые не были реализованы Андреем и добавлено несколько методов, которые по моему скромному усмотрению весьма востребованы.

### Теперь по порядку
- Support Standard Markdown / CommonMark and GFM(GitHub Flavored Markdown);
- Full-featured: Real-time Preview, Image (cross-domain) upload, Preformatted text/Code blocks/Tables insert, Code fold, Search replace, Read only, Themes, Multi-languages, L18n, HTML entities, Code syntax highlighting...;
- Markdown Extras : Support ToC (Table of Contents), Emoji, Task lists, @Links...;
- Compatible with all major browsers (IE8+), compatible Zepto.js and iPad;
- Support identification, interpretation, fliter of the HTML tags;
- Support TeX (LaTeX expressions, Based on KaTeX), Flowchart and Sequence Diagram of Markdown extended syntax;
- Support AMD/CMD (Require.js & Sea.js) Module Loader, and Custom/define editor plugins;