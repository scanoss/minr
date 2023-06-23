Name:       minr
Version:    MINR_VERSION
Release:    1%{?dist}
Summary:    SCANOSS Minr
License:    GPLv2
BuildArch:  x86_64

%description
Minr is a multi-purpose command line tool used for data mining. Minr downloads, indexes and imports source code metadata into the knowledge base. Minr can be also used locally for detecting licences and cryptographic algorithms usage.

%prep

%build

%install
mkdir -p %{buildroot}/%{_bindir}
install -m 0755 %{name} %{buildroot}/%{_bindir}/%{name}

%files
%{_bindir}/%{name}

%changelog