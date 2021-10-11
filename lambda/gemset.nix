{
  aws_lambda_ric = {
    groups = ["default"];
    platforms = [];
    source = {
      remotes = ["https://rubygems.org"];
      sha256 = "19c4xlgnhgwf3n3z57z16nmr76jd2vihhshknm5zqip2g00awhi1";
      type = "gem";
    };
    version = "2.0.0";
  };
  mustermann = {
    dependencies = ["ruby2_keywords"];
    groups = ["default"];
    platforms = [];
    source = {
      remotes = ["https://rubygems.org"];
      sha256 = "0m70qz27mlv2rhk4j1li6pw797gmiwwqg02vcgxcxr1rq2v53rnb";
      type = "gem";
    };
    version = "2.0.2";
  };
  rack = {
    groups = ["default"];
    platforms = [];
    source = {
      remotes = ["https://rubygems.org"];
      sha256 = "0axc6w0rs4yj0pksfll1hjgw1k6a5q0xi2lckh91knfb72v348pa";
      type = "gem";
    };
    version = "2.2.4";
  };
  rack-protection = {
    dependencies = ["rack"];
    groups = ["default"];
    platforms = [];
    source = {
      remotes = ["https://rubygems.org"];
      sha256 = "169jzzgvbjrqmz4q55wp9pg4ji2h90mggcdxy152gv5vp96l2hgx";
      type = "gem";
    };
    version = "2.2.2";
  };
  ruby2_keywords = {
    groups = ["default"];
    platforms = [];
    source = {
      remotes = ["https://rubygems.org"];
      sha256 = "1vz322p8n39hz3b4a9gkmz9y7a5jaz41zrm2ywf31dvkqm03glgz";
      type = "gem";
    };
    version = "0.0.5";
  };
  serverless-rack = {
    dependencies = ["rack"];
    groups = ["default"];
    platforms = [];
    source = {
      fetchSubmodules = false;
      rev = "7364305bcbbf7f6cc6851497069a5a4cb91936b1";
      sha256 = "0c7ch0s0nl70p6ijg7q0jnq8ca2rhp5wqfp91kai81dy7d71mq65";
      type = "git";
      url = "https://github.com/logandk/serverless-rack";
    };
    version = "1.0.7";
  };
  sinatra = {
    dependencies = ["mustermann" "rack" "rack-protection" "tilt"];
    groups = ["default"];
    platforms = [];
    source = {
      remotes = ["https://rubygems.org"];
      sha256 = "0mbjp75dy35q796iard8izsy7gk55g2c3q864r2p13my3yjmlcvz";
      type = "gem";
    };
    version = "2.2.2";
  };
  tilt = {
    groups = ["default"];
    platforms = [];
    source = {
      remotes = ["https://rubygems.org"];
      sha256 = "186nfbcsk0l4l86gvng1fw6jq6p6s7rc0caxr23b3pnbfb20y63v";
      type = "gem";
    };
    version = "2.0.11";
  };
}
