# Той самий пакет у форматі RPM: метадані й інструкція збірки в одному файлі.
Name:           lab09-pkgtools
Version:        1.1
Release:        1%{?dist}
Summary:        Package version comparison and installed-package audit
License:        MIT
URL:            https://github.com/yevhentolstoluzkyi-code/CS1232026
Source0:        %{name}-%{version}.tar.gz
BuildRequires:  gcc, make

%description
vercmp compares Debian package versions exactly like dpkg --compare-versions.
pkgaudit reads the dpkg database and reports disk usage, dependencies and
leaf packages.

%prep
%autosetup

%build
%make_build vercmp pkgaudit CFLAGS="%{optflags}"

%install
%make_install

%files
%license debian/copyright
%{_bindir}/vercmp
%{_bindir}/pkgaudit
%config(noreplace) %{_sysconfdir}/pkgaudit.conf

%changelog
* Wed Nov 25 2026 Denys Nadutyi <denys.nadutyi@student.karazin.ua> - 1.1-1
- Package build for comparison with Debian format
