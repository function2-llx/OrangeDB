create database test;
use test;
show databases;
create table a(a int not null, b int, c varchar(20));
create table b(a int, b int, c date);
show tables;
desc a;
desc b;
alter table a add primary key(a);
alter table b add primary key(a);