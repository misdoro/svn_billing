INSERT INTO `allzones` (`id`, `ip`, `mask`, `dstport`, `comment`) VALUES
(1, 0, 0, 0, 'All internet'),
(2, 3246212608, 23, 0, 'sandy'),
(3, 3267276800, 20, 0, 'sandy'),
(5, 3279609856, 19, 0, 'sandy'),
(7, 3641848832, 23, 0, 'sandy'),
(16, 1505558528, 19, 0, 'Nizegorod Teleservis'),
(9, 1539489792, 23, 0, 'sandy'),
(15, 3648412928, 24, 0, 'ads'),
(14, 3562831872, 18, 0, 'ads'),
(13, 3277987840, 19, 0, 'ads'),
(17, 3642167296, 20, 0, 'Nizegorod Teleservis'),
(18, 1435435008, 20, 0, 'NNGU'),
(19, 1495058176, 24, 0, 'NNGU'),
(21, 3561160704, 19, 0, 'NIS'),
(22, 3642314752, 22, 0, 'PSS'),
(23, 1432076288, 21, 0, 'PSS'),
(24, 1333657600, 17, 0, 'VT'),
(25, 1389379584, 18, 0, 'VT'),
(26, 1500315648, 18, 0, 'VT'),
(27, 3585171456, 19, 0, 'VT (but 213.177.96.6/32, 213.177.96.8/32, 213.177.96.9/32,213.177.96.221/32, 213.177.97.26/32)'),
(28, 3585171462, 32, 0, 'VT - Payable'),
(29, 3585171464, 32, 0, 'VT - Payable'),
(30, 3585171465, 32, 0, 'VT - Payable'),
(31, 3585171677, 32, 0, 'VT - Payable'),
(32, 3585171738, 32, 0, 'VT - Payable');

--
-- Дамп данных таблицы `groupnames`
--

INSERT INTO `groupnames` (`id`, `caption`) VALUES
(1, 'All internet'),
(2, 'ixNN');

--
-- Дамп данных таблицы `usergroups`
--

INSERT INTO `usergroups` (`id`, `user_id`, `group_id`) VALUES
(1, 1, 1),
(2, 1, 2),
(3, 2, 1),
(4, 2, 2),
(17, 9, 1),
(6, 3, 2),
(7, 4, 1),
(8, 4, 2),
(9, 5, 1),
(10, 5, 2),
(11, 6, 1),
(12, 6, 2),
(13, 7, 1),
(14, 7, 2),
(15, 8, 1),
(16, 8, 2),
(18, 9, 2),
(19, 10, 1),
(20, 10, 2),
(21, 11, 1),
(22, 11, 2),
(23, 3, 1);

--
-- Дамп данных таблицы `users`
--

INSERT INTO `users` (`id`, `login`, `password`, `user_ip`, `allowed`, `ip_digit`, `debit`, `credit`) VALUES
(2, 'test', '', '192.168.8.1', 1, 0, 100, 100),
(3, 'batanik', '', '192.168.8.3', 1, 0, 98.0986, 100),
(4, 'dread', '', '192.168.8.4', 1, 0, 100, 100),
(5, 'misdoro', '', '192.168.6.99', 1, 0, 99.7915, 100),
(6, 'vitalvann', '', '192.168.8.5', 1, 0, 99.9881, 100),
(7, 'murad', '', '192.168.8.6Â ', 1, 0, 100, 100),
(8, 'big_ben', '', '192.168.8.7', 1, 0, 98.1158, 100),
(9, 'pavlin', '', '192.168.8.8', 1, 0, 96.7951, 100),
(10, 'bill3', '', '192.168.8.9', 1, 0, 99.7633, 100),
(11, 'michael', '', '192.168.8.10', 1, 0, 100, 100);

--
-- Дамп данных таблицы `zone_groups`
--

INSERT INTO `zone_groups` (`id`, `zone_id`, `group_id`, `mb_cost`, `priority`) VALUES
(1, 1, 1, 1, 0),
(2, 28, 1, 1, 1000),
(3, 29, 1, 1, 1000),
(4, 30, 1, 1, 1000),
(5, 31, 1, 1, 1000),
(6, 32, 1, 1, 1000),
(7, 2, 2, 0, 500),
(8, 3, 2, 0, 500),
(9, 5, 2, 0, 500),
(10, 7, 2, 0, 500),
(11, 9, 2, 0, 500),
(12, 13, 2, 0, 500),
(13, 14, 2, 0, 500),
(14, 15, 2, 0, 500),
(15, 16, 2, 0, 500),
(16, 17, 2, 0, 500),
(17, 18, 2, 0, 500),
(18, 19, 2, 0, 500),
(19, 21, 2, 0, 500),
(20, 22, 2, 0, 500),
(21, 23, 2, 0, 500),
(22, 24, 2, 0, 500),
(23, 25, 2, 0, 500),
(24, 26, 2, 0, 500),
(25, 27, 2, 0, 500);