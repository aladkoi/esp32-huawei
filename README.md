 Для  работы "умного тормоза" необходимо получать импульсы с датчика хола - белый провод на 6 контактном разъеме датчиков холла контроллера.
 Датчики хола имеют открытый коллектор, поэтому необходима подтяжка с 5 вольт на 10кОм. Данный сигнал через 1кОм поступает на 19 контакт контроллера.
 Таймер esp32 каждую секунду считает количество импульсов, если колесо вращается, то после отпускания тормоза автоматически идет возврат к предыдущей скорости.
 Если колесо остановлено и импульсов с датчика скорости нет, то 4 секунды после отпускания ручки тормоза скорость не восстанавливается и нужно начать движение 
 с нажатие на кнопку +.
