# ЛР 9. Сітнікова, КІ-31

## 1. Середовище виконання

- Система: Ubuntu 26.04 LTS (resolute) у WSL 2 на Windows
- Ядро Linux: 6.18.33.2-microsoft-standard-WSL2
- GCC: gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0
- dpkg: 1.23.7
- apt: 3.2.1
- Частину 4 виконано двічі: у WSL 2 і повторно в контейнері (Docker 29.1.3)

## 2. Перевірка автоматичними тестами

Для перевірки було виконано:

```
python3 ../../../tools/lab09-test.py .
```

Результат:

```
OK   t_vercmp_basic
OK   t_vercmp_tilde
OK   t_vercmp_epoch_rev
OK   t_vercmp_invalid
OK   t_vercmp_dpkg
OK   t_vercmp_sort
OK   t_audit_stats
OK   t_audit_deps
OK   t_audit_leaves
OK   t_audit_provides
OK   t_audit_diff
OK   t_deb_meta
OK   t_deb_postrm
OK   t_deb_reproducible

14/14
```

Усі 14 автоматичних тестів пройдено успішно.

## 3. Відповіді на питання до звіту.

### 3.1. Питання 1.

З епохою в моїй системі є `bash-completion 1:2.16.0-8build1` і `bsdutils 1:2.41.3-3ubuntu2.2`, з тильдою — `apparmor 5.0.0~beta1-0ubuntu7` і `gzip 1.14-1~exp2ubuntu1.1`.

У `xz-utils 5.8.3-1ubuntu0.1` епохи немає (вона дорівнює 0), версія автора — `5.8.3`, ревізія — `1ubuntu0.1`: `1` — ревізія Debian, `ubuntu0.1` — зміна Ubuntu поверх неї. Відкат до коду 5.4.5 після виявлення бекдору в 5.6.0 і 5.6.1 не можна було записати меншою версією: apt не вважав би такий пакет оновленням. Тому відкат записали як `5.6.1+really5.4.5`: це більше за `5.6.1` (`dpkg --compare-versions 5.6.1+really5.4.5 gt 5.6.1` дав "так"), а вміст — код 5.4.5. Епоху для цього не використали, бо вона лишилася б назавжди й зламала б порівняння всіх майбутніх версій.

Пара `git-man (>> 1:2.53.0), git-man (<< 1:2.53.0-.)` допускає версії, строго більші за `1:2.53.0` і менші за `1:2.53.0-.`, тобто будь-які ревізії саме випуску `1:2.53.0` (наприклад, `1:2.53.0-1ubuntu1`), але не саму `1:2.53.0` без ревізії і не `1:2.53.1`. Верхню межу взято `-.`, бо `.` має вагу після всіх літер і цифр: ревізія `.` більша за будь-яку реальну ревізію, що починається з цифри чи літери, а "найбільшої" ревізії не існує.

### 3.2. Питання 2.

За зростанням: `1.0~rc1` < `1.0` < `1.0-1~bpo1` < `1.0-1` < `1.0-1ubuntu1` < `0:1.0-2` < `1.0+dfsg` < `1:0.9`. Кроки порівняння такі. `1.0~rc1` < `1.0`: після `1.0` в одного рядок скінчився (вага 0), а в іншого тильда (вага −1, менша за кінець рядка). `1.0` < `1.0-1~bpo1`: версія автора однакова, ревізія порожня проти `1~bpo1`, а порожній цифровий шматок — це 0, тоді як там 1. `1.0-1~bpo1` < `1.0-1`: після `1` у першого тильда (−1), у другого кінець рядка (0). `1.0-1` < `1.0-1ubuntu1`: у першого кінець (0), у другого літера `u`. `1.0-1ubuntu1` < `0:1.0-2`: епохи рівні (відсутня епоха — це 0), версії автора рівні, а ревізії `1ubuntu1` і `2` розрізняються першою цифрою 1 < 2. `0:1.0-2` < `1.0+dfsg`: у версіях автора `1.0` і `1.0+dfsg` після `1.0` кінець (0) проти `+` (вага після всіх літер), тому `1.0+dfsg` більша незалежно від ревізії. `1.0+dfsg` < `1:0.9`: епоха 1 більша за 0 і вирішує до всього іншого.

`strcmp` для `1.10` і `1.9` дає неправильний результат, бо порівнює байти: на другій позиції `1` менша за `9`, тож виходить `1.10` < `1.9`. Алгоритм dpkg порівнює цифрові шматки як числа, тому 10 > 9.

Версія збірки з git від 1 листопада 2026 року — `1.0~git20261101`: тильда робить її меншою за `1.0`, а `0.9` вона більша (перевірила `./vercmp 0.9 lt 1.0~git20261101` і `./vercmp 1.0~git20261101 lt 1.0`, обидва повернули код 0).

У semver `1.0.0-rc1` менше за `1.0.0`, бо дефіс там позначає передвипускну версію. У dpkg дефіс відокремлює ревізію: `1.0-rc1` — це версія автора `1.0` з ревізією `rc1`, і вона більша за `1.0` без ревізії (`./vercmp 1.0-rc1 gt 1.0` дав код 0). Передвипускну версію в Debian записують тильдою: `1.0~rc1`.

### 3.3. Питання 3.

У системі встановлено 793 пакети, які займають 2325529 КіБ, тобто 2271,0 МіБ. Три найбільші: `llvm-21-dev` (377371 КіБ), `snapd` (142924 КіБ) і `libllvm21` (135358 КіБ). Кількість абзаців `Package:` у `/var/lib/dpkg/status` теж 793, бо всі вони у стані `install ok installed`. У загальному випадку абзаців у `status` може бути більше, бо там лишаються й пакети в інших станах, наприклад `deinstall ok config-files` (після `remove` пакет стає саме таким, у `dpkg -l` це `rc`), а `pkgaudit stats` рахує лише встановлені.

Вручну я встановила 42 пакети (`apt-mark showmanual`). Решту apt вважає встановленою автоматично: у `/var/lib/apt/extended_states` у 751 запису стоїть `Auto-Installed: 1`, а 793 − 751 = 42.

### 3.4. Питання 4.

Усього листків 12 (168323 КіБ, тобто 164,4 МіБ). Три найбільші: `valgrind` (85042 КіБ), `gh` (36244 КіБ) і `linux-perf` (36106 КіБ). Усі вони встановлені вручну (apt-mark showmanual), тобто явною командою apt install, а не підтягнуті як залежності: це інструменти розробки, які я встановила під час виконання лабораторних робіт (valgrind — перевірка пам'яті, gh — клієнт GitHub, linux-perf — профілювальник).

У тестовій базі `lab09-test.py` пакет `postfix` має `Provides: mail-transport-agent`, а від цього віртуального пакета залежить `app-a`. Тому за назвою від `postfix` ніхто не залежить, але через `Provides` він потрібен, і `mark_used` його позначає.

Листок — це пакет, від якого не залежить жоден інший встановлений пакет. `apt autoremove` прибирає інше: пакети, які встановлено автоматично і які вже ніхто не потребує. На моїй системі `apt-get -s autoremove` не прибрав би жодного пакета (0 рядків `Remv`), а листків 12, і всі вони встановлені вручну, тож `autoremove` їх не чіпає. Пакет, який встановлюють вручну, майже завжди листок, бо залежності спрямовані від програми до бібліотек, а не навпаки: кінцевий інструмент ніщо не підтягує.

### 3.5. Питання 5.

Моє `Depends` — `libc6 (>= 2.38)`. `dpkg-shlibdeps` бере найвищу версію символів glibc зі всіх бінарників. У `pkgaudit` це `__isoc23_sscanf` і `__isoc23_strtol` (`GLIBC_2.38`), а `vercmp` потребує лише `GLIBC_2.34` (`__libc_start_main`), тож загальна вимога визначається `pkgaudit`. Якщо замінити `#define _GNU_SOURCE` на `#define _POSIX_C_SOURCE 200809L`, символи `__isoc23_*` зникають, і `Depends` стає `libc6 (>= 2.34)`. `_GNU_SOURCE` вмикає в заголовках glibc, зокрема, поведінку C23 (наприклад, strtol розуміє префікс 0b), і виклики перенаправляються на __isoc23_*, а з _POSIX_C_SOURCE лишаються старі символи. Мінімальну версію бібліотеки визначають не автор пакета, а версії символів, які фактично використано.

У каркасі `lintian` знайшов п'ять помилок чотирьох видів. `arch-independent-package-contains-binary-or-object` (двічі): `Architecture: all`, але в пакеті скомпільовані програми. `description-too-short` і `extended-description-is-empty`: опис залишився `TODO` без розгорнутої частини. `file-in-etc-not-marked-as-conffile`: `/etc/pkgaudit.conf` не вказано в `conffiles`, тож `dpkg` не стежив би за змінами адміністратора. У моїй версії лишилися лише чотири попередження. `uses-dpkg-database-directly` означає, що `pkgaudit` читає `/var/lib/dpkg/status` напряму: формат внутрішній, і Debian радить користуватися `dpkg-query` та іншими інструментами; я читаю файл свідомо, бо це завдання роботи. `no-manual-page` (двічі): для програм у `/usr/bin` немає сторінки man. `initial-upload-closes-no-bugs`: перший запис changelog не закриває звіту про помилку (ITP), потрібного для завантаження в Debian.

### 3.6. Питання 6.

Без `SOURCE_DATE_EPOCH` дві збірки того самого коду дали різні хеші (`828fc77c16b2ac65` і `6dbe367bf6b9f45b`), а з нею обидві — `28cd4483732238e5`. Відрізняються часові мітки: у повторному вимірі для версії 1.1-1 `ar tv` показує для обох пакетів лише хвилину створення (`Nov 16 11:17`), а `tar --full-time` для даних — `11:17:33–34` у першого пакета й `11:17:41` у другого; через це відрізняються розміри `control.tar.xz` (1248 і 1244 байти) і `data.tar.xz` (11364 і 11376 байтів), а отже й хеш (`aa21c6014737f4db` проти `c0d674ba267bcf8a`).

Дату беруть з `changelog`, бо це частина вихідного коду пакета, її фіксує автор при випуску версії, і вона доступна в архіві без історії git, а дата останнього коміту змінюється від будь-якої правки і недоступна в тарболі. Відтворюваність дозволяє будь-кому незалежно зібрати пакет із тих самих джерел і порівняти хеші: якщо до зібраного пакета потрапило щось, чого немає у відкритих джерелах (у випадку xz шкідливий код був у збірочних скриптах релізного архіву, але не в git), збірки розійдуться, і це змусило б шукати причину.

### 3.7. Питання 7.

У `Packages` порівняно з `DEBIAN/control` з'явилися поля `Filename` (шлях до `.deb` у репозиторії, щоб apt знав, що завантажувати), `Size` (розмір файла для перевірки повноти завантаження) і контрольні суми `MD5sum`, `SHA1`, `SHA256`, `SHA512` (за ними apt перевіряє завантажений `.deb`; MD5 і SHA1 застарілі, apt позначає їх `[weak]`). Решта полів (`Package`, `Version`, `Architecture`, `Maintainer`, `Installed-Size`, `Depends`, `Description` тощо) — копії з `control`.

Підписано `InRelease`: у ньому суми й розміри `Packages` і `Packages.gz`. У `Packages` є суми `.deb`. Ланцюг такий: ключ → підпис `InRelease` → хеш `Packages` → хеш `.deb`. Змінити будь-яку ланку, не зламавши попередню, неможливо.

`Signed-By` безпечніший за спільну в'язку ключів `trusted.gpg`, бо в `trusted.gpg` довіра до ключа діє для всіх репозиторіїв: ключ мого репозиторію міг би підписати підроблений пакет Ubuntu. З `Signed-By: /etc/apt/keyrings/lab09-repo.gpg` цей ключ перевіряє лише джерело `lab09`.

### 3.8. Питання 8.

Після оновлення `ls /etc/pkgaudit.conf*` показав два файли: `/etc/pkgaudit.conf` (мій змінений варіант з `keep = vim`) і `/etc/pkgaudit.conf.dpkg-dist` (нова версія від пакета). Так вийшло, бо конфігураційний файл я змінила після встановлення, а пакет 1.1-1 приніс інший варіант; з `--force-confold` dpkg лишає мій файл, а новий кладе поруч із суфіксом `.dpkg-dist`. Без `--force-confold` dpkg запитав би інтерактивно, що робити (залишити, замінити, показати різницю). Якби я не змінювала конфіг, dpkg мовчки замінив би його на нову версію, і `.dpkg-dist` не з'явився б.

`pkgaudit diff` показав `+ htop 3.4.1-5build2`, `^ lab09-pkgtools 1.0-1 -> 1.1-1` і підсумок `added 1 removed 0 upgraded 1 downgraded 0`. `htop` у моїй системі не приніс додаткових пакетів: усі його залежності вже були встановлені.

Після `remove` пакет у стані `rc` (видалено, конфігурація лишилася): файли `/etc/pkgaudit.conf*` і каталог `/var/lib/pkgaudit` зі знімком залишилися. `purge` прибрав конфігураційні файли (разом з `.dpkg-dist`), а мій `postrm` під час `purge` видалив `/var/lib/pkgaudit`, про який dpkg не знає: після `purge` обидва шляхи зникли.

### 3.9. Питання 9.

1. Змінила рядок `Suite` в `InRelease`: apt написав `The following signatures were invalid: BADSIG C388D70A91D30560 Lab09 repo (Sitnikova) ...` і лишив старі індекси. Виявив підпис у `InRelease`.
2. Змінила версію в `Packages` (і `Packages.gz`) на `9.9-1`: apt написав `Hash Sum mismatch` для `Packages.gz` (очікуваний SHA256 `bccf6924…`, отриманий `d15deb14…`). Виявила хеш `Packages`, записаний у підписаному `InRelease`.
3. Дописала байт до `.deb` у `pool/`: apt повернув `Unable to fetch some archives` з розбіжністю хешів (отриманий файл на байт більший, `Filesize:12813`). Виявила контрольна сума (`SHA256`/`SHA512`) і `Size` у `Packages`.

Ланцюг не захищає від того, що дані старі: якщо зловмисник роздає старий, але чесно підписаний `InRelease`, підпис справжній, і перевірка пройде, якщо в `Release` немає поля `Valid-Until`, яке обмежує строк придатності. Він також не захищає від компрометації самого ключа і від шкідливого пакета, підписаного справжнім ключем.

Якби мій ключ потрапив у pull request, його треба вважати скомпрометованим: будь-хто зміг би підписати пакет від мого імені. Ключ треба негайно відкликати (сертифікат відкликання лежить у `~/.gnupg/openpgp-revocs.d/`), прибрати з репозиторію, створити новий ключ і перевипустити репозиторій. Видалення з історії git лише ускладнює доступ, але не скасовує витік.

### 3.10. Питання 10.

У `.deb` залежність записано одним полем `Depends: libc6 (>= 2.38)`: ім'я пакета й мінімальна версія, а потрібну версію обирає `dpkg-shlibdeps` за найвищим символом. У моєму `.rpm` `rpm -qp --requires` перелічує кожну потрібну можливість окремо: `libc.so.6()(64bit)`, `libc.so.6(GLIBC_2.14)(64bit)`, `libc.so.6(GLIBC_2.2.5)(64bit)`, `libc.so.6(GLIBC_2.3)(64bit)`, `libc.so.6(GLIBC_2.3.4)(64bit)`, `libc.so.6(GLIBC_2.34)(64bit)`, `libc.so.6(GLIBC_2.38)(64bit)`, `libc.so.6(GLIBC_2.4)(64bit)`, `rtld(GNU_HASH)`, а також `rpmlib(...)` і `config(lab09-pkgtools) = 1.0-1`. Менеджер RPM (dnf) шукає пакет, який серед своїх `Provides` має потрібну можливість (наприклад, `libc.so.6(GLIBC_2.38)(64bit)`), а apt шукає пакет за ім'ям `libc6` і перевіряє версію. Помилка `can't create transaction lock on /var/lib/rpm/.rpm.lock` нешкідлива: в Ubuntu немає бази RPM.

Секції `.spec` відповідають моїм файлам так. `%prep` (`%autosetup`) розпаковує тарбол із джерелами; у Debian цього кроку в моєму `Makefile` немає, бо збираю прямо з робочого каталогу. `%build` (`%make_build vercmp pkgaudit`) відповідає цілям, що компілюють програми (`cc … vercmp.c debver.c`, `cc … pkgaudit.c debver.c`). `%install` (`%make_install`) відповідає цілі `install` з `DESTDIR=build/root` (`install -D …`). `%files` відповідає вмісту `data.tar` (`/usr/bin/vercmp`, `/usr/bin/pkgaudit`, `/etc/pkgaudit.conf` і ліцензія з `debian/copyright`). `%config(noreplace) %{_sysconfdir}/pkgaudit.conf` відповідає рядку `/etc/pkgaudit.conf` у `debian/conffiles`: при оновленні, якщо адміністратор змінив файл, RPM лишає його, а новий кладе поруч як `.rpmnew` (аналог `.dpkg-dist`).

## 4. Таблиці вимірювань

### 4.1. 5.5. Вимінювання

|            Показник            |      Ваша система      |  Контейнер/BM       |
|-------------------------------:|-----------------------:|--------------------:|
| Пакетів (installed)	         | 793	                  | 423                 |
|-------------------------------:|-----------------------:|--------------------:|
| Місця, МіБ (total_mib)	 | 2271,0	          | 581,9               |
|-------------------------------:|-----------------------:|--------------------:|
| Листків і їхній розмір, МіБ	 | 12 / 164,4	          | 5 / 28,6            |
|-------------------------------:|-----------------------:|--------------------:|
| Пакетів, встановлених вручну	 | 42	                  | 99                  |
|-------------------------------:|-----------------------:|--------------------:|
|               	         | valgrind (85042 КіБ),  | git (21696 КіБ),    |
| Три найбільші листки           | gh (36244 КіБ),        | lintian (6958 КіБ), |
|                                | linux-perf (36106 КіБ) | rpm (520 КіБ)       |
|-------------------------------:|-----------------------:|--------------------:|

Значення для контейнера ubuntu:24.04 зняті до встановлення мого пакета, але після встановлення інструментів збірки (build-essential, dpkg-dev, lintian, apt-utils, gnupg, python3, git, rpm): через це серед листків контейнера саме ці інструменти, а «встановлених вручну» там 99. У контейнері стандартний мінімальний образ, тому пакетів і місця значно менше, ніж у WSL.

### 4.2. 6.4. Вимінювання: відтворюваність

|            Збірка          |	sha256 (перші 16 символів) |
|---------------------------:|----------------------------:|
| з SOURCE_DATE_EPOCH, перша | 28cd4483732238e5            |
|---------------------------:|----------------------------:|
| з SOURCE_DATE_EPOCH, друга | 28cd4483732238e5            |
|---------------------------:|----------------------------:|
| без нього, перша	     | 828fc77c16b2ac65            |
|---------------------------:|----------------------------:| 
| без нього, друга	     | 6dbe367bf6b9f45b            |
|---------------------------:|----------------------------:| 

Ці хеші виміряно для версії 1.0-1. Повторний вимір для версії 1.1-1 (без SOURCE_DATE_EPOCH) дав aa21c6014737f4db і c0d674ba267bcf8a: теж різні.

## 5. Вивід dpkg-deb -I і lintian

Каркас (dpkg-deb -I):

```
Package: lab09-pkgtools
 Version: 1.0-1
 Architecture: all
 Maintainer: Ім'я Прізвище <you@example.org>
 Installed-Size: 62
 Depends: libc6
 Section: admin
 Priority: optional
 Description: TODO
```

Каркас (lintian):

```
E: lab09-pkgtools: arch-independent-package-contains-binary-or-object [usr/bin/pkgaudit]
E: lab09-pkgtools: arch-independent-package-contains-binary-or-object [usr/bin/vercmp]
E: lab09-pkgtools: description-too-short TODO
E: lab09-pkgtools: extended-description-is-empty
E: lab09-pkgtools: file-in-etc-not-marked-as-conffile [etc/pkgaudit.conf]
W: lab09-pkgtools: file-missing-in-md5sums etc/pkgaudit.conf [md5sums]
W: lab09-pkgtools: initial-upload-closes-no-bugs [usr/share/doc/lab09-pkgtools/changelog.Debian.gz:1]
W: lab09-pkgtools: maintainer-script-empty [postrm]
W: lab09-pkgtools: no-manual-page [usr/bin/pkgaudit]
W: lab09-pkgtools: no-manual-page [usr/bin/vercmp]
W: lab09-pkgtools: uses-dpkg-database-directly [usr/bin/pkgaudit]
```

Моя версія (dpkg-deb -I lab09-pkgtools_1.1-1_amd64.deb):

```
 Package: lab09-pkgtools
 Version: 1.1-1
 Architecture: amd64
 Maintainer: Sofia Sitnikova <sofiia.sitnikova@student.karazin.ua>
 Installed-Size: 62
 Depends: libc6 (>= 2.38)
 Section: admin
 Priority: optional
 Description: tools for inspecting Debian package versions and installed packages
  vercmp compares Debian package versions the way dpkg does.
  pkgaudit reads the dpkg database, lists installed packages and finds
  leaf packages that no other installed package depends on.
  .
  Educational package for the "System software" course.
```

Моя версія (lintian):

```
W: lab09-pkgtools: initial-upload-closes-no-bugs [usr/share/doc/lab09-pkgtools/changelog.Debian.gz:1]
W: lab09-pkgtools: no-manual-page [usr/bin/pkgaudit]
W: lab09-pkgtools: no-manual-page [usr/bin/vercmp]
W: lab09-pkgtools: uses-dpkg-database-directly [usr/bin/pkgaudit]
```

## 6. Фрагменти виводу частини 4

Перед установленням:

```
$ apt policy lab09-pkgtools
lab09-pkgtools:
  Installed: (none)
  Candidate: 1.0-1
  Version table:
     1.0-1 500
        500 file:/srv/lab09-repo ./ Packages
```

Оновлення до 1.1-1 (sudo apt -o Dpkg::Options::=--force-confold install --only-upgrade lab09-pkgtools):

```
Unpacking lab09-pkgtools (1.1-1) over (1.0-1) ...
Setting up lab09-pkgtools (1.1-1) ...

Configuration file '/etc/pkgaudit.conf'
 ==> Modified (by you or by a script) since installation.
 ==> Package distributor has shipped an updated version.
 ==> Using current old file as you requested.
```

Конфігураційні файли після оновлення:

```
$ ls /etc/pkgaudit.conf*
/etc/pkgaudit.conf  /etc/pkgaudit.conf.dpkg-dist
$ diff /etc/pkgaudit.conf /etc/pkgaudit.conf.dpkg-dist
12,13c12
< keep = openssh-server sudo
< keep = vim
---
> keep = openssh-server sudo lab09-pkgtools
```

Після встановлення htop:

```
$ pkgaudit diff
+ htop 3.4.1-5build2
^ lab09-pkgtools 1.0-1 -> 1.1-1
summary added 1 removed 0 upgraded 1 downgraded 0
```

Три підробки:

```
1. InRelease (змінено рядок Suite):
Err:1 file:/srv/lab09-repo ./ InRelease
  The following signatures were invalid: BADSIG C388D70A91D30560 Lab09 repo (Sitnikova) <sofiia.sitnikova@student.karazin.ua>
Warning: An error occurred during the signature verification. The repository is not updated and the previous index files will be used.

2. Packages (версію змінено на 9.9-1):
Err:2 file:/srv/lab09-repo ./ Packages
  Hash Sum mismatch
  Hashes of expected file:
   - SHA256:bccf6924ed5463fd614e91899dbaaca64dcc7f5be5b21368d612ac3ecfdb1ed3
  Hashes of received file:
   - SHA256:d15deb1460e13c131c39f5b14d5dde14cd83c62f5764034341df3bad32b9e2e4
Error: Failed to fetch file:/srv/lab09-repo/./Packages.gz  Hash Sum mismatch

3. .deb (до файла дописано один байт):
   Hashes of received file:
    - SHA256:57c39205fa09ae69100139e9e7bf092515836b7d0de72f402d614c9eaf830fbd
    - Filesize:12813 [weak]
Error: Unable to fetch some archives, maybe run apt update or try with --fix-missing?
```

Після remove і purge:

```
$ dpkg -l lab09-pkgtools | tail -1
rc  lab09-pkgtools 1.1-1        amd64        tools for inspecting Debian package versions and installed packages
$ ls /etc/pkgaudit.conf* /var/lib/pkgaudit        (після remove)
/etc/pkgaudit.conf  /etc/pkgaudit.conf.dpkg-dist
/var/lib/pkgaudit:
snapshot
$ ls /etc/pkgaudit.conf* /var/lib/pkgaudit        (після purge)
ls: cannot access '/etc/pkgaudit.conf*': No such file or directory
ls: cannot access '/var/lib/pkgaudit': No such file or directory
```

Усю частину 4 я повторила в контейнері одним скриптом (docker run --rm ... ubuntu:24.04 bash /ctr.sh): збірка і тести, пакет, підписаний репозиторій, установлення, три підробки, remove і purge. Репозиторій з роботою монтується в контейнер лише для читання, а збірка йде в його копії, тому в моєму каталозі нічого не змінюється, а після виходу з контейнера все встановлене зникає.

```
14/14                                   (lab09-test.py в контейнері)
 Architecture: amd64
 Installed-Size: 58
 Depends: libc6 (>= 2.38)

$ apt-cache policy lab09-pkgtools
lab09-pkgtools:
  Installed: (none)
  Candidate: 1.1-1
  Version table:
     1.1-1 500
        500 file:/srv/lab09-repo ./ Packages
Setting up lab09-pkgtools (1.1-1) ...
verify код 0                            (dpkg --verify lab09-pkgtools)
snapshot 424 /var/lib/pkgaudit/snapshot

1. InRelease:
Err:1 file:/srv/lab09-repo ./ InRelease
  The following signatures were invalid: BADSIG 2528CC947F6E075C Lab09 repo (Sitnikova) <sofiia.sitnikova@student.karazin.ua>
W: An error occurred during the signature verification. The repository is not updated and the previous index files will be used.

2. Packages:
Err:2 file:/srv/lab09-repo ./ Packages
  Hash Sum mismatch
E: Failed to fetch file:/srv/lab09-repo/./Packages.gz  Hash Sum mismatch

3. .deb:
Err:1 file:/srv/lab09-repo ./ lab09-pkgtools 1.1-1
  Hash Sum mismatch
   - Filesize:12772 [weak]
   - Filesize:12773 [weak]

$ dpkg -l lab09-pkgtools | tail -1       (після remove)
rc  lab09-pkgtools 1.1-1        amd64        tools for inspecting Debian package versions and installed packages
/etc/pkgaudit.conf
/var/lib/pkgaudit:
snapshot
ls: cannot access '/etc/pkgaudit.conf*': No such file or directory       (після purge)
ls: cannot access '/var/lib/pkgaudit': No such file or directory
```

Результати збігаються з WSL: ті самі повідомлення apt для трьох підробок (BADSIG, Hash Sum mismatch для Packages.gz, Hash Sum mismatch для .deb з розміром на байт більшим), dpkg --verify повертає 0, після remove лишається стан rc з конфігурацією і знімком, а purge прибирає все, зокрема /var/lib/pkgaudit завдяки postrm. Depends у пакеті з контейнера теж libc6 (>= 2.38), а Installed-Size менший (58 КіБ проти 62), бо в ubuntu:24.04 інші версії компілятора та strip. Ключ у контейнері створювався окремо (C265D5C2…), тож відрізняються й ідентифікатори ключа в повідомленнях. Сценарій оновлення 1.0-1 → 1.1-1 з --force-confold і htop я виконувала лише в WSL, бо в репозиторій контейнера опубліковано тільки версію 1.1-1.

## 7. Зведена таблиця результатів

|         Показник              |                Значення                 |
|------------------------------:|----------------------------------------:|
| Система, ядро (uname -r),     | Ubuntu 26.04 LTS (resolute);            |
| версії gcc, dpkg, apt;        | 6.18.33.2-microsoft-standard-WSL2;      |
| де виконано ч. 4	        | gcc 15.2.0; dpkg 1.23.7; apt 3.2.1;     |
|                               | частина 4 у WSL 2 і                     |
|                               | повторно в контейнері ubuntu:24.04      |
|------------------------------:|----------------------------------------:|
| Контейнер ubuntu:24.04:       | 423 / 581,9 / 5; 14/14;                 |
| пакетів / МіБ / листків;      | libc6 (>= 2.38)                         |
| тести; Depends пакета         |                                         |
|------------------------------:|----------------------------------------:|
| lab09-test.py: результат      | 14/14                                   |
|------------------------------:|----------------------------------------:|  
| Пакетів / МіБ /               | 793 / 2271,0 / 12                       |
| листків на вашій системі      |                                         |
|------------------------------:|----------------------------------------:|
| Depends вашого пакета;        | libc6 (>= 2.38); 12812 байтів; 62 КіБ   |
| розмір .deb і Installed-Size  |                                         |
|------------------------------:|----------------------------------------:|
| lintian: каркас / ваша версія | 5 помилок і 6 попереджень / 0 помилок   |
|                               | і 4 попередження                        |
|------------------------------:|----------------------------------------:|
| Хеші двох збірок з            | з ним 28cd4483732238e5 і                |
| SOURCE_DATE_EPOCH і без       | 28cd4483732238e5 (збігаються);          | 
| нього (збігаються?)           | без нього 828fc77c16b2ac65 і            |
|                               | 6dbe367bf6b9f45b (ні)                   |
|------------------------------:|----------------------------------------:|
| pkgaudit diff після           | + htop 3.4.1-5build2,                   |
| оновлення: рядки й підсумок   | ^ lab09-pkgtools 1.0-1 -> 1.1-1;        | 
|                               | added 1 removed 0 upgraded 1            |
|                               | downgraded 0                            |
|------------------------------:|----------------------------------------:|
| Повідомлення apt для          | BADSIG; Hash Sum mismatch (Packages);   |
| трьох підробок                | Unable to fetch some archives           |
|                               | (змінений .deb)                         |
|------------------------------:|----------------------------------------:|
| rpm -qp --requires:           | GLIBC_2.14, 2.2.5, 2.3,                 |
| рядки з GLIBC                 | 2.3.4, 2.34, 2.38, 2.4                  |
|------------------------------:|----------------------------------------:|

## 8. Висновки

Під час лабораторної роботи я реалізувала алгоритм, яким dpkg порівнює версії, і перевірила його на чотирьохстах випадкових парах; прочитала базу dpkg і знайшла у своїй системі листки, від яких ніщо не залежить; зібрала пакет з каталогу debian/, створила підписаний репозиторій, оновила через нього пакет, не втративши налаштувань адміністратора, і переконалася, що зміна одного байта в будь-якому файлі репозиторію не проходить непоміченою.

Якби мені треба було поширити власну утиліту на 50 серверів кафедри, я б створила внутрішній apt-репозиторій, як у частині 4: підписаний InRelease, відкритий ключ на серверах у /etc/apt/keyrings з Signed-By лише для цього джерела, а приватний ключ тримала б тільки на збірній машині. Пакет збирала б відтворювано (SOURCE_DATE_EPOCH з changelog), щоб будь-хто міг перевірити хеш, а конфігурацію оголосила б у conffiles, щоб оновлення не затирало налаштування адміністраторів. Установлювала б і оновлювала лише через репозиторій. Щоб переконатися, що на серверах не з'явилося нічого зайвого, перед розгортанням я б робила pkgaudit snapshot, а після — pkgaudit diff: там мають бути тільки мій пакет і очікувані залежності (як + htop і ^ lab09-pkgtools у моєму випробуванні); цілісність файлів пакета перевірила б dpkg --verify lab09-pkgtools, а джерело і версію — apt policy.

## 9. Використання ШІ

Сервіс: Claude (Anthropic), листопад 2026. Що зробив: пояснював поняття (версії dpkg, залежності, формат пакета, підпис репозиторію, RPM), пояснював коди в програмах debver.c, debian/control, conffiles, postrm.
