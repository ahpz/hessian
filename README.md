# hessian
C语言 实现JsonRpc协议PHP 扩展 （低层 使用mcpack协议）
//service.php 服务端代码

class Service {
public function add($a, $b) {
	return $a+$b;
}
};
$service = new McpackHessianService();
$service->registerObject(new Service());
$service->service();
//客户端

$obj = new McpackHessianClient('http://xxxx/service.php');
$ret = $obj->add(9,2);
var_dump($ret);exit //输出 11
